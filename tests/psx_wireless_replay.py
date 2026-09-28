#!/usr/bin/env python3

import csv
import sys
from dataclasses import dataclass, field


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def parse_hex(value):
    """Convert a CSV hex field into a list of byte values."""
    if value is None:
        return []

    value = value.strip()

    if not value:
        return []

    # Accept spaces, commas, and optional 0x prefixes.
    value = value.replace(",", " ")
    parts = value.split()

    result = []

    for part in parts:
        part = part.strip()

        if part.lower().startswith("0x"):
            part = part[2:]

        try:
            result.append(int(part, 16))
        except ValueError:
            pass

    return result


def fmt_bytes(data):
    return " ".join(f"{b:02X}" for b in data)


def command_name(cmd):
    names = {
        0x40: "GetStatus",
        0x41: "GetCapabilities",
        0x42: "Poll",
        0x43: "EnterConfig",
        0x44: "SetLock",
        0x45: "GetLEDState",
        0x46: "Config46",
        0x47: "Config47",
        0x4C: "Config4C",
        0x4D: "Config4D",
        0x4F: "SetButtonInfo",
    }

    return names.get(cmd, f"Unknown_{cmd:02X}")


# ---------------------------------------------------------------------------
# Transaction reconstruction
# ---------------------------------------------------------------------------

def find_transactions(tx, rx):
    """
    Split the captured wireless stream into transactions.

    A transaction starts when the controller sends 01h and the
    corresponding received byte is FFh.

    The initial FFh from RX is removed from the returned RX data.
    """

    transactions = []

    i = 0

    while i < min(len(tx), len(rx)):
        # Transaction start:
        #
        #   TX = 01
        #   RX = FF
        #
        if tx[i] == 0x01 and rx[i] == 0xFF:
            start = i
            i += 1

            # Continue until the next 01/FF transaction boundary.
            while i < min(len(tx), len(rx)):
                if tx[i] == 0x01 and rx[i] == 0xFF:
                    break

                i += 1

            end = i

            transaction_tx = tx[start:end]
            transaction_rx = rx[start:end]

            # The first RX byte is the initial FF handshake byte.
            if transaction_rx and transaction_rx[0] == 0xFF:
                transaction_rx = transaction_rx[1:]

            transactions.append(
                {
                    "start": start,
                    "end": end,
                    "tx": transaction_tx,
                    "rx": transaction_rx,
                }
            )

        else:
            i += 1

    return transactions


# ---------------------------------------------------------------------------
# PAD state
# ---------------------------------------------------------------------------

@dataclass
class PadState:
    """
    Reconstructed logical state of the controller/PADMAN interaction.

    Important distinction:

        pressure_capable
            Controller advertises pressure support.

        pressure_enabled
            Pressure mode has actually been entered/configured.

    We deliberately do NOT infer pressure_enabled merely from 0x41.
    """

    mode: str = "UNKNOWN"

    locked: int = 0

    pressure_capable: bool = False
    pressure_enabled: bool = False

    button_info_mask: int | None = None

    # Number of pressure-related configuration values observed.
    pressure_values_seen: int = 0

    # Keep a history for debugging.
    history: list = field(default_factory=list)

    def snapshot(self):
        return {
            "mode": self.mode,
            "locked": self.locked,
            "pressure_capable": self.pressure_capable,
            "pressure_enabled": self.pressure_enabled,
            "button_info_mask": self.button_info_mask,
            "pressure_values_seen": self.pressure_values_seen,
        }


# ---------------------------------------------------------------------------
# State machine
# ---------------------------------------------------------------------------

class PadStateMachine:

    def __init__(self):
        self.state = PadState()

    def transition(self, reason):
        self.state.history.append(
            (
                self.state.mode,
                self.state.locked,
                self.state.pressure_capable,
                self.state.pressure_enabled,
                self.state.button_info_mask,
                reason,
            )
        )

    def process(self, transaction):
        """
        Process one complete wireless transaction.

        Returns a list of human-readable observations/anomalies.
        """

        tx = transaction["tx"]
        rx = transaction["rx"]

        observations = []

        if len(tx) < 2:
            observations.append("TX too short to contain command")
            return observations

        if tx[0] != 0x01:
            observations.append(
                f"unexpected TX header {tx[0]:02X}"
            )
            return observations

        cmd = tx[1]

        # ---------------------------------------------------------------
        # 0x41
        #
        # Controller capabilities.
        #
        # For the DS2 capability response we expect pressure capability
        # to be advertised somewhere in the capability bytes. We don't
        # blindly assume a fixed response layout here; instead we inspect
        # the observed response for the known FF FF 03 capability pattern.
        # ---------------------------------------------------------------

        if cmd == 0x41:
            if len(rx) >= 3:
                if rx[0:3] == [0xFF, 0xFF, 0x03]:
                    if not self.state.pressure_capable:
                        observations.append(
                            "pressure capability detected: FF FF 03"
                        )

                    self.state.pressure_capable = True

                    observations.append(
                        "IMPORTANT: capability != pressure mode"
                    )

            else:
                observations.append(
                    "0x41 response too short for capability inspection"
                )

        # ---------------------------------------------------------------
        # 0x42
        #
        # Normal/config polling.
        #
        # We don't generate a response yet. Instead, validate what the
        # real capture actually returned against the current state.
        # ---------------------------------------------------------------

        elif cmd == 0x42:

            if self.state.mode == "CONFIG":
                observations.append(
                    "0x42 while CONFIG mode"
                )

                if rx:
                    observations.append(
                        f"config 0x42 response starts {rx[0]:02X}"
                    )

                    if rx[0] != 0xF3:
                        observations.append(
                            "WARNING: CONFIG 0x42 does not start with F3"
                        )

            else:
                observations.append(
                    f"0x42 while {self.state.mode} mode"
                )

            # Pressure sanity check.
            #
            # We intentionally don't assert an exact byte count yet.
            # Instead we record whether pressure mode has been established.
            if self.state.pressure_capable:
                if self.state.pressure_enabled:
                    observations.append(
                        "pressure-capable + pressure-enabled: "
                        "pressure fields are expected in poll response"
                    )
                else:
                    observations.append(
                        "pressure-capable but pressure mode NOT enabled"
                    )

        # ---------------------------------------------------------------
        # 0x43
        #
        # Config-mode entry / initialization operation.
        # ---------------------------------------------------------------

        elif cmd == 0x43:

            old_mode = self.state.mode

            self.state.mode = "CONFIG"

            if old_mode != self.state.mode:
                observations.append(
                    f"state transition: {old_mode} -> CONFIG"
                )

        # ---------------------------------------------------------------
        # 0x44
        #
        # Set lock state.
        #
        # The MCU-side handler we traced does:
        #
        #     spi->locked = spi->dma_buf[4];
        #
        # so TX byte 4 is the important field when present.
        # ---------------------------------------------------------------

        elif cmd == 0x44:

            if len(tx) > 4:
                new_locked = tx[4]

                if new_locked != self.state.locked:
                    observations.append(
                        f"lock: {self.state.locked:02X} -> "
                        f"{new_locked:02X}"
                    )

                self.state.locked = new_locked

            else:
                observations.append(
                    "0x44 too short to inspect lock byte"
                )

        # ---------------------------------------------------------------
        # 0x45
        #
        # Get LED/controller information.
        #
        # We record it but don't infer more state than we have proven.
        # ---------------------------------------------------------------

        elif cmd == 0x45:

            observations.append(
                "0x45 controller/LED information request"
            )

            if len(rx) >= 2:
                observations.append(
                    f"0x45 response: {fmt_bytes(rx)}"
                )

        # ---------------------------------------------------------------
        # 0x4F
        #
        # SetButtonInfo / pressure configuration.
        #
        # This is the critical transition we're investigating.
        # ---------------------------------------------------------------

        elif cmd == 0x4F:

            observations.append(
                "*** 0x4F SetButtonInfo observed ***"
            )

            # Display complete command because the exact semantics of
            # each field are what we're reverse engineering.
            observations.append(
                f"0x4F TX: {fmt_bytes(tx)}"
            )

            # Look for the known 0xFFF button-info mask.
            #
            # We search the payload rather than assuming an offset,
            # because we are deliberately still validating the exact
            # RPC/SPI marshalling.
            mask_found = False

            for i in range(2, len(tx) - 2):
                if tx[i] == 0xFF and tx[i + 1] == 0x0F:
                    mask_found = True
                    self.state.button_info_mask = 0x0FFF
                    break

                if tx[i] == 0xFF and tx[i + 1] == 0xFF:
                    # Possible little-endian 16-bit 0xFFFF field.
                    pass

            if mask_found:
                observations.append(
                    "0x0FFF button-info mask detected"
                )

                if self.state.pressure_capable:
                    self.state.pressure_enabled = True

                    observations.append(
                        "STATE CHANGE: pressure mode ENABLED"
                    )
                else:
                    observations.append(
                        "0x4F seen before pressure capability "
                        "was established"
                    )

            else:
                observations.append(
                    "0x4F seen, but 0x0FFF mask was not identified"
                )

        # ---------------------------------------------------------------
        # Other config commands
        # ---------------------------------------------------------------

        elif cmd in (0x46, 0x47, 0x4C, 0x4D):

            observations.append(
                f"config command 0x{cmd:02X}"
            )

        else:
            observations.append(
                f"unhandled command 0x{cmd:02X}"
            )

        self.transition(
            f"command 0x{cmd:02X} ({command_name(cmd)})"
        )

        return observations


# ---------------------------------------------------------------------------
# Sanity checks
# ---------------------------------------------------------------------------

def sanity_check(number, transaction, state_before, state_after, observations):
    """
    Additional checks which don't alter state.
    """

    tx = transaction["tx"]
    rx = transaction["rx"]

    warnings = []

    if len(tx) >= 2:
        cmd = tx[1]

        # ---------------------------------------------------------------
        # Polling before capability discovery
        # ---------------------------------------------------------------

        if cmd == 0x42 and not state_after["pressure_capable"]:
            warnings.append(
                "poll occurred before pressure capability was observed"
            )

        # ---------------------------------------------------------------
        # Pressure polling after 0x4F
        # ---------------------------------------------------------------

        if cmd == 0x42 and state_after["pressure_enabled"]:
            warnings.append(
                "pressure-enabled poll: inspect response length/content"
            )

        # ---------------------------------------------------------------
        # 0x4F without capability
        # ---------------------------------------------------------------

        if cmd == 0x4F and not state_before["pressure_capable"]:
            warnings.append(
                "0x4F occurred before 0x41 established capability"
            )

    return warnings



# ---------------------------------------------------------------------------
# Replay model
# ---------------------------------------------------------------------------

class ReplayModel:

    """
    Generates the response that the reconstructed state machine currently
    believes should be returned.

    IMPORTANT:

    This first implementation intentionally does NOT invent complete
    controller packets. It only knows structural properties that we have
    established from the trace.

    As we verify the remaining fields, expected_response() can be expanded
    until this becomes a complete software implementation of the protocol.
    """

    def expected_response(self, transaction, state):
        tx = transaction["tx"]

        if len(tx) < 2:
            return None

        cmd = tx[1]

        # For now, return None for commands where we have not completely
        # reconstructed the response.
        #
        # This is deliberate: "unknown" is much more useful than silently
        # generating a wrong packet.

        if cmd == 0x42:
            if state.mode == "CONFIG":
                return {
                    "known": True,
                    "prefix": [0xF3],
                    "description": "CONFIG poll response"
                }

            return {
                "known": False,
                "description": "NORMAL poll response not yet reconstructed"
            }

        if cmd == 0x41:
            return {
                "known": False,
                "description": "0x41 response layout not yet fully reconstructed"
            }

        if cmd == 0x45:
            return {
                "known": False,
                "description": "0x45 response layout not yet fully reconstructed"
            }

        if cmd == 0x4F:
            return {
                "known": False,
                "description": "0x4F response not yet reconstructed"
            }

        return None


# ---------------------------------------------------------------------------
# Trace output
# ---------------------------------------------------------------------------

def print_state(state):
    mask = (
        "----"
        if state.button_info_mask is None
        else f"{state.button_info_mask:04X}"
    )

    print(
        "  STATE:"
        f" mode={state.mode}"
        f" lock={state.locked:02X}"
        f" pressure_capable={'YES' if state.pressure_capable else 'NO'}"
        f" pressure_enabled={'YES' if state.pressure_enabled else 'NO'}"
        f" button_mask={mask}"
    )


def process_transactions(transactions):
    machine = PadStateMachine()
    replay = ReplayModel()

    for number, transaction in enumerate(transactions, 1):

        tx = transaction["tx"]
        rx = transaction["rx"]

        before = machine.state.snapshot()

        if len(tx) >= 2:
            cmd = tx[1]
            name = command_name(cmd)
        else:
            cmd = None
            name = "INVALID"

        print("=" * 78)
        print(
            f"Transaction {number} "
            f"offset {transaction['start']}..{transaction['end'] - 1}"
        )

        print(
            f"  COMMAND: "
            f"{name}"
            if cmd is not None
            else "  COMMAND: INVALID"
        )

        print(f"  TX: {fmt_bytes(tx)}")
        print(f"  RX: {fmt_bytes(rx)}")

        observations = machine.process(transaction)

        after = machine.state.snapshot()

        warnings = sanity_check(
            number,
            transaction,
            before,
            after,
            observations,
        )

        print()

        for observation in observations:
            print(f"  * {observation}")

        for warning in warnings:
            print(f"  ! {warning}")

        print()

        print("  BEFORE:")
        print(
            f"    mode={before['mode']}"
            f" lock={before['locked']:02X}"
            f" pressure_capable="
            f"{'YES' if before['pressure_capable'] else 'NO'}"
            f" pressure_enabled="
            f"{'YES' if before['pressure_enabled'] else 'NO'}"
        )

        print("  AFTER:")
        print_state(machine.state)

        # ---------------------------------------------------------------
        # Replay model
        # ---------------------------------------------------------------

        expected = replay.expected_response(
            transaction,
            machine.state,
        )

        if expected is not None:

            if expected.get("known"):
                prefix = expected.get("prefix", [])

                actual_prefix = rx[:len(prefix)]

                if actual_prefix == prefix:
                    print(
                        f"  REPLAY: PASS "
                        f"({expected['description']})"
                    )
                else:
                    print(
                        f"  REPLAY: FAIL "
                        f"({expected['description']})"
                    )

                    print(
                        f"    expected prefix: {fmt_bytes(prefix)}"
                    )

                    print(
                        f"    actual prefix:   {fmt_bytes(actual_prefix)}"
                    )

            else:
                print(
                    f"  REPLAY: UNKNOWN "
                    f"({expected['description']})"
                )

        print()


# ---------------------------------------------------------------------------
# CSV loading
# ---------------------------------------------------------------------------

def load_csv(filename):

    tx = []
    rx = []

    with open(
        filename,
        "r",
        newline="",
        encoding="utf-8-sig"
    ) as f:

        reader = csv.reader(f)

        header_found = False

        for row in reader:

            if not row:
                continue

            # The CSV headers are literally:
            #
            #   wireless cmd
            #   wireless data
            #
            if not header_found:

                headers = [
                    x.strip().lower()
                    for x in row
                ]

                if (
                    "wireless cmd" in headers
                    and
                    "wireless data" in headers
                ):
                    cmd_index = headers.index(
                        "wireless cmd"
                    )

                    data_index = headers.index(
                        "wireless data"
                    )

                    header_found = True
                    continue

            if not header_found:
                continue

            if (
                cmd_index >= len(row)
                or
                data_index >= len(row)
            ):
                continue

            cmd_bytes = parse_hex(
                row[cmd_index]
            )

            data_bytes = parse_hex(
                row[data_index]
            )

            tx.extend(cmd_bytes)
            rx.extend(data_bytes)

    if not header_found:
        print("Could not find CSV headers:")
        print("  wireless cmd")
        print("  wireless data")
        sys.exit(1)

    return tx, rx


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():

    if len(sys.argv) != 2:
        print(
            f"Usage: {sys.argv[0]} <csv-file>"
        )
        sys.exit(1)

    filename = sys.argv[1]

    tx, rx = load_csv(filename)

    print(f"Captured TX bytes: {len(tx)}")
    print(f"Captured RX bytes: {len(rx)}")
    print()

    transactions = find_transactions(
        tx,
        rx
    )

    print(
        f"Found {len(transactions)} transactions"
    )

    print()

    process_transactions(
        transactions
    )


if __name__ == "__main__":
    main()
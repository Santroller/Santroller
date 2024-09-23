#include <string.h>
#include <stdio.h>
#include "pico/stdio.h"

#include "psxSPI.pio.h"
#include "controller_simulator.h"
#include "pico/multicore.h"



// volatile PSXInputState *inputState;

// static uint8_t mode = MODE_DIGITAL;
// static bool config = false;
// static bool analogLock = false;
// static uint8_t motorBytes[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
// static uint8_t pollConfig[4] = { 0x00, 0x00, 0x00, 0x00 };


// void cancel_ack() {
// 	pio_sm_exec(psx_device_pio, smCmdReader, pio_encode_jmp(offsetCmdReader));		// restart smCmdReader
// }

// void SEND(uint8_t byte) {
// 	write_byte_blocking(psx_device_pio, smDatWriter, byte);
// }

// uint8_t RECV_CMD() {
// 	return read_byte_blocking(psx_device_pio, smCmdReader);
// }

// void initController() {
// 	mode = MODE_DIGITAL;
// 	config = false;
// 	analogLock = false;
// 	memset(motorBytes, 0xFF, sizeof(motorBytes));
// 	pollConfig[0] = 0xFF;
// 	pollConfig[1] = 0xFF;
// 	pollConfig[2] = 0x03;
// 	pollConfig[3] = 0x00;
// }

// void processRumble(uint8_t index, uint8_t value) {
// 	/*if(index == 0 || index > 5)
// 		return;
// 	switch(motorBytes[index - 1]) {
// 		case 0x00: {
// 			outputState.rumbleRight = value ? 0xFF : 0x00;
// 			break;
// 		}
// 		case 0x01: {
// 			outputState.rumbleLeft = value;
// 			break;
// 		}
// 	}*/
// }


// //0x40
// void processPresConfig() {
// 	if (!config) {
// 		return;
// 	}
// 	uint8_t buf[7] = { 0x5A, 0x00, 0x00, 0x02, 0x00, 0x00, 0x5A };
// 	for(uint8_t i = 0; i < 7; i++)
// 	{
// 		SEND(buf[i]);
// 		RECV_CMD();
// 	}
// }
// //0x41
// void processPollConfigStatus() {
// 	if (!config) {
// 		return;
// 	}
// 	uint8_t buf[7] = { 0x5A, (mode == MODE_DIGITAL) ? 0x00 : 0xFF, (mode == MODE_DIGITAL) ? 0x00 : 0xFF, (mode == MODE_DIGITAL) ? 0x00 : 0x03, (mode == MODE_DIGITAL) ? 0x00 : 0x00, 0x00, (mode == MODE_DIGITAL) ? 0x00 : 0x5A };
// 	for(uint8_t i = 0; i < 7; i++) {
// 		SEND(buf[i]);
// 		RECV_CMD();
// 	}
// }
// //0x42
// void processPoll() {
// 	config = false;
// 	switch(mode) {
// 		case MODE_DIGITAL: {
// 			uint8_t buf[3] = { 0x5A, inputState->buttons1, inputState->buttons2 };
// 			for(uint8_t i = 0; i < 3; i++) {
// 				SEND(buf[i]);
// 				processRumble(i, RECV_CMD());
// 			}
// 			break;
// 		}
// 		case MODE_ANALOG: {
// 			uint8_t buf[7] = { 0x5A,inputState->buttons1, inputState->buttons2, inputState->rx, inputState->ry, inputState->lx, inputState->ly };
// 			for(uint8_t i = 0; i < 7; i++) {
// 				SEND(buf[i]);
// 				processRumble(i, RECV_CMD());
// 			}
// 			break;
// 		}
// 		case MODE_ANALOG_PRESSURE: {
// 			uint8_t buf[19] = { 0x5A, inputState->buttons1, inputState->buttons2, inputState->rx, inputState->ry, inputState->lx, inputState->ly, (inputState->buttons1 & RIGHT) ? 0x00 : 0xFF, (inputState->buttons1 & LEFT) ? 0x00 : 0xFF, 
// 							   (inputState->buttons1 & UP) ? 0x00 : 0xFF, (inputState->buttons1 & DOWN) ? 0x00 : 0xFF , (inputState->buttons2 & TRI) ? 0x00 : 0xFF, (inputState->buttons2 & CIR) ? 0x00 : 0xFF,
// 							   (inputState->buttons2 & X) ? 0x00 : 0xFF, (inputState->buttons2 & SQU) ? 0x00 : 0xFF, (inputState->buttons2 & L1) ? 0x00 : 0xFF, (inputState->buttons2 & R1) ? 0x00 : 0xFF, inputState->l2, inputState->r2 };
// 			for(uint8_t i = 0; i < 19; i++) {
// 				SEND(buf[i]);
// 				processRumble(i, RECV_CMD());
// 			}
// 			break;
// 		}
// 	}
// }
// //0x43
// void processConfig() {
// 	switch(config ? MODE_CONFIG : mode) {
// 		case MODE_CONFIG: {
// 			for(uint8_t i = 0; i < 7; i++) {
// 				SEND((i == 0) ? 0x5A : 0x00);
// 				if(i != 1)
// 					RECV_CMD();
// 				else
// 					config = RECV_CMD();
// 			}
// 			break;
// 		}
// 		case MODE_DIGITAL: {
// 			uint8_t buf[3] = { 0x5A, inputState->buttons1, inputState->buttons2 };
// 			for(uint8_t i = 0; i < 3; i++) {
// 				SEND(buf[i]);
// 				if(i != 1)
// 					RECV_CMD();
// 				else
// 					config = RECV_CMD();
// 			}
// 			break;
// 		}
// 		case MODE_ANALOG: {
// 			uint8_t buf[7] = { 0x5A, inputState->buttons1, inputState->buttons2, inputState->rx, inputState->ry, inputState->lx, inputState->ly };
// 			for(uint8_t i = 0; i < 7; i++) {
// 				SEND(buf[i]);
// 				if(i != 1)
// 					RECV_CMD();
// 				else
// 					config = RECV_CMD();
// 			}
// 			break;
// 		}
// 		case MODE_ANALOG_PRESSURE: {
// 			uint8_t buf[19] = { 0x5A, inputState->buttons1, inputState->buttons2, inputState->rx, inputState->ry, inputState->lx, inputState->ly, (inputState->buttons1 & RIGHT) ? 0xFF : 0x00, (inputState->buttons1 & LEFT) ? 0xFF : 0x00, 
// 							   (inputState->buttons1 & UP) ? 0xFF : 0x00, (inputState->buttons1 & DOWN) ? 0xFF : 0x00 , (inputState->buttons2 & TRI) ? 0xFF : 0x00, (inputState->buttons2 & CIR) ? 0xFF : 0x00,
// 							   (inputState->buttons2 & X) ? 0xFF : 0x00, (inputState->buttons2 & SQU) ? 0xFF : 0x00, (inputState->buttons2 & L1) ? 0xFF : 0x00, (inputState->buttons2 & R1) ? 0xFF : 0x00, inputState->l2, inputState->r2 };
// 			for(uint8_t i = 0; i < 19; i++) {
// 				SEND(buf[i]);
// 				if(i != 1)
// 					RECV_CMD();
// 				else
// 					config = RECV_CMD();
// 			}
// 			break;
// 		}
// 	}
// }

// uint8_t detectAnalog() {
// 	if((pollConfig[0] + pollConfig[1] + pollConfig[2] + pollConfig[3]) > 0) {
// 		return MODE_ANALOG_PRESSURE;
// 	} else {
// 		return MODE_ANALOG;
// 	}
// }

// //0x44
// void processAnalogSwitch() {
// 	if (!config) {
// 		return;
// 	}
// 	for(uint8_t i = 0; i < 7; i++) {
// 		switch(i) {
// 			case 0: {
// 				SEND(0x5A);
// 				RECV_CMD();
// 				break;
// 			}
// 			case 1: {
// 				SEND(0x00);
// 				mode = (RECV_CMD() == 0x01) ? detectAnalog() : MODE_DIGITAL;
// 				break;
// 			}
// 			case 2: {
// 				SEND(0x00);
// 				analogLock = (RECV_CMD() == 0x03) ? 1 : 0;
// 				break;
// 			}
// 			default: {
// 				SEND(0x00);
// 				RECV_CMD();
// 			}
// 		}
// 	}
// }
// //0x45
// void processStatus() {
// 	if (!config) {
// 		return;
// 	}
// 	uint8_t buf[7] = { 0x5A, 0x03, 0x02, (mode == MODE_DIGITAL) ? 0x00 : 0x01, 0x02, 0x01, 0x00 };
// 	for(uint8_t i = 0; i < 7; i++) {
// 		SEND(buf[i]);
// 		RECV_CMD();
// 	}
// }
// //0x46
// void processConst46() {
// 	if (!config) {
// 		return;
// 	}
// 	SEND(0x5A);
// 	RECV_CMD();
// 	SEND(0x00);
// 	uint8_t offset = RECV_CMD();
// 	uint8_t buf[5] = { 0x00, 0x01, (offset == 0x00) ? 0x02 : 0x01, (offset == 0x00) ? 0x00 : 0x01, 0x0F };
// 	for(uint8_t i = 0; i < 5; i++) {
// 		SEND(buf[i]);
// 		RECV_CMD();
// 	}
// }
// //0x47
// void processConst47() {
// 	if (!config) {
// 		return;
// 	}
// 	uint8_t buf[7] = { 0x5A, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00 };
// 	for(uint8_t i = 0; i < 7; i++) {
// 		SEND(buf[i]);
// 		RECV_CMD();
// 	}
// }
// //0x4C
// void processConst4c() {
// 	if (!config) {
// 		return;
// 	}
// 	SEND(0x5A);
// 	RECV_CMD();
// 	SEND(0x00);
// 	uint8_t offset = RECV_CMD();
// 	uint8_t buf[5] = { 0x00, 0x00, (offset == 0x00) ? 0x04 : 0x07, 0x00, 0x00 };
// 	for(uint8_t i = 0;i < 5;i++) {
// 		SEND(buf[i]);
// 		RECV_CMD();
// 	}
// }
// //0x4D
// void processEnableRumble() {
// 	if (!config) {
// 		return;
// 	}
// 	for(uint8_t i = 0; i < 7; i++) {
// 		if(i == 0) {
// 			SEND(0x5A);
// 			RECV_CMD();
// 		} else {
// 			SEND(motorBytes[i - 1]);
// 			motorBytes[i - 1] = RECV_CMD();
// 		}
// 	}
// }
// //0x4F
// void processPollConfig() {
// 	if (!config) {
// 		return;
// 	}
// 	for(int i = 0; i < 7; i++) {
// 		if(i >= 1 && i <= 4) {
// 			SEND((0x00));
// 			pollConfig[i-1] = RECV_CMD();
// 		} else {
// 			SEND((i == 0 || i == 6) ? 0x5A : 0x00);
// 			RECV_CMD();
// 		}
// 	}
// 	if((pollConfig[0] + pollConfig[1] + pollConfig[2] + pollConfig[3]) != 0) {
// 		mode = MODE_ANALOG_PRESSURE;
// 	} else {
// 		mode = MODE_ANALOG;
// 	}
// }


// void process_joy_req() {
// 	SEND(config ? MODE_CONFIG : mode);
// 	uint8_t cmd = RECV_CMD();
// 	switch(cmd) {
// 		case(CMD_POLL): {
// 			processPoll();
// 			break;
// 		}
// 		case(CMD_CONFIG): {
// 			processConfig();
// 			break;
// 		}
// 		case(CMD_STATUS): {
// 			processStatus();
// 			break;
// 		}
// 		case(CMD_CONST_46): {
// 			processConst46();
// 			break;
// 		}
// 		case(CMD_CONST_47): {
// 			processConst47();
// 			break;
// 		}
// 		case(CMD_CONST_4C): {
// 			processConst4c();
// 			break;
// 		}
// 		case(CMD_POLL_CONFIG_STATUS): {
// 			processPollConfigStatus();
// 			break;
// 		}
// 		case(CMD_ENABLE_RUMBLE): {
// 			processEnableRumble();
// 			break;
// 		}
// 		case(CMD_POLL_CONFIG): {
// 			processPollConfig();
// 			break;
// 		}
// 		case(CMD_PRES_CONFIG): {
// 			processPresConfig();
// 			break;
// 		}
// 		case(CMD_ANALOG_SWITCH): {
// 			processAnalogSwitch();
// 			break;
// 		}
// 		default: {
// 			//printf("Unknown CMD: 0x%.2x\n", cmd);
// 			break;
// 		}
// 	}
// }


// void psx_device_main() {
// 	while(true) {
// 		if(RECV_CMD() == 0x01) {
// 			process_joy_req();
// 		}
// 	}
// }


// void __time_critical_func(sel_isr_callback()) {
//     // TODO refractor comment, also is __time_critical_func needed for speed? we should test if everything works without it!
//     /* begin inlined call of:  gpio_acknowledge_irq(PIN_ATT, GPIO_IRQ_EDGE_RISE); kept in RAM for performance reasons */
//     check_gpio_param(PIN_ATT);
//     io_bank_0_hw->intr[PIN_ATT / 8] = GPIO_IRQ_EDGE_RISE << (4 * (PIN_ATT % 8));
//     /* end of inlined call */
//     restart_pio_sm();
// }



// void psx_device_init(uint pio, PSXInputState *data, void (*reset_pio) ()) {

// 	psx_device_pio = pio ? pio1 : pio0;
	
// 	/*Data from gamepad*/
// 	inputState = data;

// 	/*Core 1 function*/
// 	core1_function = reset_pio;

// 	init_pio();

// 	gpio_set_slew_rate(PIN_DAT, GPIO_SLEW_RATE_FAST);
// 	gpio_set_drive_strength(PIN_DAT, GPIO_DRIVE_STRENGTH_12MA);

// 	gpio_set_irq_enabled(PIN_ATT, GPIO_IRQ_EDGE_RISE, true);
//     //irq_set_exclusive_handler(IO_IRQ_BANK0, sel_isr_callback); // instead of normal gpio_set_irq_callback() which has slower handling
//     irq_add_shared_handler(IO_IRQ_BANK0, sel_isr_callback, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
//     irq_set_enabled(IO_IRQ_BANK0, true);


// 	printf("Init psx simulator\n");
// }


static PIO psx_device_pio = pio1; 
void (*core1_function)() = {0};  
uint smCmdReader;
uint smDatWriter;

uint offsetCmdReader;
uint offsetDatWriter;
void init_pio() {

	gpio_set_dir(PIN_DAT, false);
	gpio_set_dir(PIN_CMD, false);
	gpio_set_dir(PIN_ATT, false);
	gpio_set_dir(PIN_CLK, false);
	gpio_set_dir(PIN_ACK, false);
	gpio_disable_pulls(PIN_DAT);
	gpio_disable_pulls(PIN_CMD);
	gpio_disable_pulls(PIN_ATT);
	gpio_disable_pulls(PIN_CLK);
	gpio_disable_pulls(PIN_ACK);

	smCmdReader = pio_claim_unused_sm(psx_device_pio, true);
	smDatWriter = pio_claim_unused_sm(psx_device_pio, true);

	offsetCmdReader = pio_add_program(psx_device_pio, &cmd_reader_program);
	offsetDatWriter = pio_add_program(psx_device_pio, &dat_writer_program);

	cmd_reader_program_init(psx_device_pio, smCmdReader, offsetCmdReader);
	dat_writer_program_init(psx_device_pio, smDatWriter, offsetDatWriter);
}

void SEND(uint8_t byte) {
	write_byte_blocking(psx_device_pio, smDatWriter, byte);
}

uint8_t RECV_CMD() {
	return read_byte_blocking(psx_device_pio, smCmdReader);
}
extern uint8_t configMode;
extern uint8_t mode;
uint8_t receiveCommand() {
    uint8_t modeByte = configMode ? 0xF3 : mode;
    uint8_t ret = RECV_CMD();
    if (ret != 1) {
        return 0;
    }
	SEND(mode);
    return RECV_CMD();
}
uint8_t receiveAll(uint8_t* data, uint8_t len) {
    for (int i = 0; i < len; i++) {
		SEND(data[i]);
		data[i] = RECV_CMD();
    }
    return len;
}
uint8_t receiveAll2(uint8_t* data0, uint8_t* data1, uint8_t len) {
    uint8_t* data = data0;
    for (int i = 0; i < len; i++) {
		SEND(data[i]);
		data0[i] = RECV_CMD();
        if (data0[1] == 1) {
            data = data1;
        }
    }
    return len;
}

void __time_critical_func(restart_pio_sm)() {
    pio_set_sm_mask_enabled(psx_device_pio, 1 << smCmdReader | 1 << smDatWriter, false);
    pio_restart_sm_mask(psx_device_pio, 1 << smCmdReader | 1 << smDatWriter);
    pio_sm_exec(psx_device_pio, smCmdReader, pio_encode_jmp(offsetCmdReader));	// restart smCmdReader PC
    pio_sm_exec(psx_device_pio, smDatWriter, pio_encode_jmp(offsetDatWriter));	// restart smDatWriter PC
    pio_sm_clear_fifos(psx_device_pio, smCmdReader);
    pio_sm_drain_tx_fifo(psx_device_pio, smDatWriter); // drain instead of clear, so that we empty the OSR

    // resetting and launching core1 here allows to perform the reset of the transaction (e.g. when PSX polls for new MC without completing the read)
    multicore_reset_core1();
    multicore_launch_core1(core1_function);
    pio_enable_sm_mask_in_sync(psx_device_pio, 1 << smCmdReader | 1 << smDatWriter);
}
void __time_critical_func(sel_isr_callback()) {
    // TODO refractor comment, also is __time_critical_func needed for speed? we should test if everything works without it!
    /* begin inlined call of:  gpio_acknowledge_irq(PIN_ATT, GPIO_IRQ_EDGE_RISE); kept in RAM for performance reasons */
    check_gpio_param(PIN_ATT);
    io_bank0_hw->intr[PIN_ATT / 8] = GPIO_IRQ_EDGE_RISE << (4 * (PIN_ATT % 8));
    /* end of inlined call */
    restart_pio_sm();
}
# Using the tool on linux
You will need to copy some udev rules to your computer in order to use the tool without sudo
Create the file `/etc/udev/rules.d/70-ardwiino.rules` with the following content
```
ACTION!="add|change", GOTO="ardwiino_rules_end"

SUBSYSTEM!="usb|tty|hidraw", GOTO="ardwiino_rules_end"

ATTRS{idVendor}=="1209", ATTRS{idProduct}=="2882", MODE="666", TAG+="uaccess"
ATTRS{idVendor}=="0f0d", ATTRS{idProduct}=="0092", MODE="666", TAG+="uaccess"
ATTRS{idVendor}=="12ba", ATTRS{idProduct}=="0100", MODE="666", TAG+="uaccess"
ATTRS{idVendor}=="12ba", ATTRS{idProduct}=="0200", MODE="666", TAG+="uaccess"
ATTRS{idVendor}=="12ba", ATTRS{idProduct}=="0210", MODE="666", TAG+="uaccess"
ATTRS{idVendor}=="12ba", ATTRS{idProduct}=="0120", MODE="666", TAG+="uaccess"
ATTRS{idVendor}=="1bad", ATTRS{idProduct}=="0004", MODE="666", TAG+="uaccess"
ATTRS{idVendor}=="1bad", ATTRS{idProduct}=="074b", MODE="666", TAG+="uaccess"

LABEL="ardwiino_rules_end"
```

Once that file is in place, run the following commands

```sh
sudo udevadm control --reload-rules
sudo udevadm trigger
```

If you are a Ubuntu/Debian user you may need to add yourself to to the `dialout` group

```sh
sudo usermod -a -G dialout $USER
sudo usermod -a -G plugdev $USER
```

If you are a Arch user you may need to add yourself to the `uucp` group

```sh
sudo usermod -a -G uucp $USER
sudo usermod -a -G lock $USER
```

To run the tool, simply run `./guitar-configurator-linux-v9.1.7.AppImage`

Note that if you are on wayland, you may need to run the tool with `GDK_BACKEND=x11 ./guitar-configurator-linux-v9.1.7.AppImage`.
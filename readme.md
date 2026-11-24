# 1602 LCD Display Controller
Control your 1602 display in an easy and fully customizable way!  
This repo was written for Raspberry Pi's, but should be compatible with any Linux installation with a GPIO I2C bus included to control the 1602 display.

## Features
This implementation includes:
- LCD initialization sequence
- Command execution for display control
- Customizable background daemon for continuous display management
    - Features both a main loop and a side loop; more details in `screens.c`!
- Manual message override capability, either for a set duration or until cleared

## Usage
Start the background daemon (requires sudo privileges):  
*(Alternatively, some of the make commands below also automatically start the background daemon.)*
```bash
lcd -d
```
Interrupt the background display and show a custom message:  
*(Display is truncated if a line is longer than 16 characters.)*
```bash
lcd <Line 1> <Line 2> [duration]
lcd "Line 1" "Line 2"   # If not provided, default duration is 10s
lcd "Line 1" "Line 2" 5 # Displays for 5 seconds, then resumes the background display daemon
lcd "Line 1" "Line 2" 0 # Displays until manually cleared 
```
Clear any message and resume the background display daemon:
```bash
lcd -c
```

## Make documentation
To build the project from source, simply run:
```bash
make
```
To register and start the background daemon, run:
```bash
make install
```
After editing a file (`screens.c` for instance), update the daemon using:
```bash
make update
```
Monitor the screen from the terminal:
```bash
make monitor
```
If you ever feel like uninstalling the background daemon, run:
```bash
make uninstall
```
Clear temporary files created during compilation (including the executable):
```bash
make clean
```

## Hardware Setup
**Warning !**  
The code assumes an 1602 LCD screen is connected via I2C to the Raspberry Pi.  
The specific I2C address is defined in src/main.c  
### **<ins> Please edit the #define I2C_ADDR line according to your own address!</ins>**  
Run `i2cdetect -y 1` to check your own I2C address(es).

## Requirements
- A 1602 screen and a Raspberry Pi (duh)
- GCC & Make (`build-essential` package bundle is recommended)
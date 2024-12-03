# Overview
Sample applications I wrote for a Xilinx Zynq Ultrascale+ system with both R5/baremetal and A53/linux cohexisting 
and sharing the PL resources. All examples are meant for ZCU102 Xilinx evaluation board.
The final application will be a control system with (multichannel) ADC and DAC, using PL for acquisition (~0.5 MSps max sampling rate) and fast processing (if any), 
R5 for the actual control loop (~1kHz max sampling rate) and linux on A53 for command and control. R5 gets IRQs from the PL to get samples; 
A53 and R5 communicate using openAmp (=shared memory + IPI = inter processor interrupt).

## PL System
### PL system for test 1 to 4
In the PL there are a few IPs, which mimic the resources I am interested in for the final application, like sampling clock and ADC/DAC interface; 
in this example there are:
- an AXI timer; it can generate an interrupt; it is representative of a samplink clock
- an AXI GPIO; it can generate an interrupt; it is representative of asyncronous external I/O
- a custom AXI IP with a register bank ("REGBANK")); it can generate an interrupt; it is representative of the glue logic to interface ADC and DAC

See picture of the block design:
![PL block design for sample R5 apps](https://github.com/user-attachments/assets/adc81bdd-4bd1-4589-8356-ba22d7154d88)

### PL system for test 5
A fast interrupt (FIQ) port is added for the R5 processor: it will be used to serve the timer interrupt with minimum latency.

Vivado configuration:
![image](https://github.com/user-attachments/assets/7654c99d-a96f-49e1-96cf-908f22f1e26c)

In the block design the timer IRQ is now routed (inverted) to the nFIQ pin:
![image](https://github.com/user-attachments/assets/d1bccbe7-511f-46d6-a55e-c894c508b6d3)


# List of examples
All the examples are mainly for R5/baremetal; the ones that have inter processor communication also include a linux (A53) part

## rtest1 (baremetal R5)
Read/write AXI register bank only; no interrupts

## rtest2irq1 (baremetal R5)
Like rtest1, but add a service routine for the interrupt generated by the REGBANK IP

## rtest3irq2 (baremetal R5)
Like rtest2irq1, but add axi GPIO with its interrupt

## rtest4irq3 (baremetal R5)
Like rtest3irq2, but add axi TIMER with its interrupt

## test5 (baremetal R5 + linux on A53)
Like rtest4irq3, but add inter-process communication between linux/A53 and baremetal R5. 
In addition, timer IRQ is now a fast interrupt (FIQ) and served by a dedicated ISR.

Userspace openamp is used; the code is directly derived fro the echo test example of Xilinx guide UG1186.
Please note that both sides can be debugged with vitis debugger (even at the same time), 
R5 via JTAG and linux via TCF agent, as usual. 
For deployment, the R5 application
can be added to boot image with petalinux-package (i.e. bootgen), while linux side is a normal executable,
which can be started automatically at boot, if need so.

This test is divided into two vitis application projects:
- test5-r5-userspace-openamp
- test5-a53linux-userspace-openamp

In these directories you'll find only the main.c/.h; all the rest of the code is common and can be found
 in test5-common_src directory.
It's possible to test the definition of symbol "ARMR5" at compile time, to find out whether the code
is going to R5/baremetal or A53/linux; anyway, the differences are limited to a few lines.

At boot, the R5 part starts and waits for connection (handshake) from the linux part; 
then the main loop is started, which waits for an interrupt (there is at least one every second,
due to the timer interrupt) and prints the total number of received interrupts, 
including the IPI = inter processor interrupt = incoming message from linux.
It also prints the state of the register bank
and the value of the two sample parameters (one float, one signed integer 32bit) communicated by linux side.

On the linux side, you run the app and then you are asked to enter the value of the two parameters, 
which are then sent to the R5 side. Press "e" to exit the loop and end linux application; at that point,
the R5 side receives a notification and ends cleanly as well.

In the linker script, critical code is put into TCMB to reduce timer IRQ latency to a minimum (350ns); 
also critical functions are put into TCMB with ARM \_\_attribute__(section) directive; see file r5_main.h.








# Overview
Sample applications I wrote for a Xilinx Zynq Ultrascale+ system with both R5/baremetal and A53/linux cohexisting and sharing the PL resources.
The final application will be a control system with (multichannel) ADC and DAC, using PL for acquisition (~0.5 MSps max sampling rate) and fast processing (if any), 
R5 for the actual control loop (~1kHz max sampling rate) and linux on A53 for command and control. R5 gets IRQs from the PL to get samples; 
A53 and R5 communicate using openAmp (=shared memory + IPI = inter processor interrupt).

## PL System
In the PL there are a few IPs, which mimic the resources I am interested in for the final application, like sampling clock and ADC/DAC interface; 
in this example there are:
- an AXI timer; it can generate an interrupt; it is representative of a samplink clock
- an AXI GPIO; it can generate an interrupt; it is representative of asyncronous external I/O
- a custom AXI IP with a register bank; it can generate an interrupt; it is representative of the glue logic to interface ADC and DAC

See picture of the block design:



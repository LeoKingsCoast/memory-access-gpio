# Memory Access GPIO

This program toggles the GPIO0_41 pin of a AM62x System on Chip (SoC) from
Texas Instruments by mapping the SoC registers into memory and modifying them
directly. It was made to be run on a Verdin AM62 System on Module from Toradex, 
to toggle the GPIO_8_CSI pin (SODIMM_202 on the standard Verdin pin numbering). 
This is part of the tests made to evaluate the PREEMPT_RT configuration in 
[meta-tests-preempt-rt](https://github.com/LeoKingsCoast/meta-tests-preempt-rt).

**THIS PROGRAM IS PURELY EXPERIMENTAL.** Do not use this method for controlling
GPIO pins in real projects. Additionally, do not execute this program on any 
SoC other than the AM62x, and be aware of the functionality of this pin in 
your module before running this program.

## Building and Running

1. Prepare your cross-compilation toolchain. In particular, it should set the 
compiler (`CC`) properly for your machine. This project was compiled 
originally using the [Yocto Standard SDK](https://docs.yoctoproject.org/sdk-manual/using.html). 
Alternatively, you can compile it natively if your image includes build tools.

1. Simply run `make` to build the application. Note: Building the application 
will require root access. This is necessary to give the binary the 
`CAP_IPC_LOCK` capability, required for it to lock memory pages for real-time 
safety.

    ```bash
    sudo make
    ```

1. Copy the `gpio-toggle` output binary to your module and execute it. The 
GPIO0_41 should toggle at a 1 second interval.

    ```bash
    ./gpio-toggle
    ```

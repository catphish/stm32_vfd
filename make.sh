set -e
arm-none-eabi-gcc --specs=nosys.specs -std=gnu11 -Wall -pedantic -g -O2 -mlittle-endian -mthumb -mthumb-interwork -mcpu=cortex-m4 -fsingle-precision-constant -Wdouble-promotion -c main.c -o main.o
arm-none-eabi-gcc --specs=nosys.specs -std=gnu11 -Wall -pedantic -g -O2 -mlittle-endian -mthumb -mthumb-interwork -mcpu=cortex-m4 -fsingle-precision-constant -Wdouble-promotion -c boot.s -o boot.o
arm-none-eabi-ld -o main.elf -T linker.ld boot.o main.o
arm-none-eabi-objcopy -O binary main.elf main.bin
st-flash write main.bin 0x8000000

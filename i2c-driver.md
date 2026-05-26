
# Linux I2C Driver 

# 1. Introduction

I2C stands for:

```text
Inter-Integrated Circuit
````

I2C is a synchronous serial communication protocol used for communication between processors and peripheral devices.

It is one of the most widely used communication buses in:

* Embedded Linux
* IoT devices
* Smartphones
* Automotive systems
* Industrial electronics

Linux I2C Drivers allow the kernel to communicate with external I2C slave devices such as:

* Sensors
* EEPROMs
* RTC chips
* ADC/DAC devices
* Touch controllers
* Power management ICs

---

# 2. What is an I2C Driver?

An I2C Driver is a Linux kernel driver that communicates with hardware devices connected to the I2C bus.

The driver:

* Detects I2C devices
* Sends commands
* Reads sensor values
* Transfers register data
* Handles interrupts (optional)

I2C drivers usually use:

* Device Tree
* I2C subsystem
* i2c_client structure
* i2c_driver structure

---

# 3. Why Do We Use I2C Drivers?

Without I2C drivers:

* User applications cannot safely communicate with I2C devices
* Hardware-specific register handling becomes difficult
* Bus management becomes unsafe

I2C drivers provide:

| Feature              | Purpose                           |
| -------------------- | --------------------------------- |
| Hardware abstraction | Hide low-level protocol details   |
| Bus management       | Kernel controls I2C communication |
| Standard APIs        | Reusable Linux interfaces         |
| Device Tree support  | Dynamic hardware configuration    |
| Multi-device support | Many devices share same bus       |

---

# 4. Real-Time Examples

| Device                 | Usage                |
| ---------------------- | -------------------- |
| Temperature Sensor     | Read temperature     |
| EEPROM                 | Store configuration  |
| RTC                    | Time/date management |
| Accelerometer          | Motion sensing       |
| Touchscreen Controller | Touch input          |
| PMIC                   | Power management     |
| OLED Display           | Screen control       |
| Camera Sensor          | Video capture        |

---

# 5. I2C Bus Basics

I2C uses only two wires:

| Signal | Purpose      |
| ------ | ------------ |
| SDA    | Serial Data  |
| SCL    | Serial Clock |

---

# 6. I2C Architecture

```text id="n2s3fd"
+------------------------------+
| User Space Application       |
+--------------+---------------+
               |
               v
+------------------------------+
| Linux I2C Subsystem          |
+--------------+---------------+
               |
               v
+------------------------------+
| I2C Driver                   |
|------------------------------|
| probe()                      |
| read/write registers         |
| interrupt handling           |
+--------------+---------------+
               |
               v
+------------------------------+
| I2C Adapter Controller       |
+--------------+---------------+
               |
               v
+------------------------------+
| I2C Slave Device             |
+------------------------------+
```

---

# 7. I2C Communication Flow

```text id="9g6xmp"
Master Start
    ↓
Slave Address
    ↓
Read/Write Bit
    ↓
ACK/NACK
    ↓
Data Transfer
    ↓
Stop Condition
```

---

# 8. Important Terminology

## I2C Master

Controls communication.

Example:

* CPU
* SoC

---

## I2C Slave

Responds to master requests.

Example:

* Sensor
* EEPROM

---

## I2C Address

Each slave has unique address.

Example:

```text
0x48
0x68
0x50
```

---

# 9. Linux I2C Subsystem

Linux provides a complete I2C framework.

Core structures:

```c id="1pt8jr"
struct i2c_client
struct i2c_driver
```

Important APIs:

```c id="q6t6h4"
i2c_master_send()
i2c_master_recv()
i2c_smbus_read_byte_data()
i2c_smbus_write_byte_data()
```

---

# 10. Important Header Files

```c id="8skvfc"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/delay.h>
```

---

# 11. Device Tree Example

## mydevice.dts

```dts id="bzlr2m"
&i2c1 {

    mysensor@48 {
        compatible = "myvendor,mysensor";
        reg = <0x48>;
    };
};
```

---

# 12. Device Tree Explanation

| Property    | Purpose           |
| ----------- | ----------------- |
| i2c1        | I2C controller    |
| mysensor@48 | Device node       |
| compatible  | Driver matching   |
| reg         | I2C slave address |

---

# 13. I2C Driver Flow

## Step 1 – Device Tree Match

Kernel matches:

```c id="qyw7nf"
compatible = "myvendor,mysensor"
```

with driver.

---

## Step 2 – probe() Called

Kernel calls:

```c id="z0w63s"
probe()
```

---

## Step 3 – I2C Communication Begins

Driver accesses hardware registers.

---

## Step 4 – Read/Write Data

Using Linux I2C APIs.

---

# 14. Full I2C Driver Example

## i2c_driver.c

```c id="1zndyx"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/of.h>

#define MY_REG 0x10

static int my_i2c_probe(struct i2c_client *client,
                        const struct i2c_device_id *id)
{
    int ret;

    printk(KERN_INFO "I2C Driver Probe Called\n");

    /* Read one byte from register 0x10 */
    ret = i2c_smbus_read_byte_data(client, MY_REG);

    if (ret < 0) {
        printk(KERN_ERR "I2C Read Failed\n");
        return ret;
    }

    printk(KERN_INFO "Register Value = 0x%x\n", ret);

    return 0;
}

static void my_i2c_remove(struct i2c_client *client)
{
    printk(KERN_INFO "I2C Driver Removed\n");
}

static const struct of_device_id my_i2c_of_match[] = {
    { .compatible = "myvendor,mysensor" },
    { }
};

MODULE_DEVICE_TABLE(of, my_i2c_of_match);

static const struct i2c_device_id my_i2c_id[] = {
    { "mysensor", 0 },
    { }
};

MODULE_DEVICE_TABLE(i2c, my_i2c_id);

static struct i2c_driver my_i2c_driver = {
    .driver = {
        .name = "my_i2c_driver",
        .of_match_table = my_i2c_of_match,
    },

    .probe = my_i2c_probe,
    .remove = my_i2c_remove,
    .id_table = my_i2c_id,
};

module_i2c_driver(my_i2c_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Simple Linux I2C Driver");
```

---

# 15. Makefile

```Makefile id="7r9n9j"
obj-m += i2c_driver.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
```

---

# 16. Compile Driver

```bash id="bnw7ig"
make
```

Output:

```bash id="mzyhlk"
i2c_driver.ko
```

---

# 17. Insert Driver

```bash id="4m1a8z"
sudo insmod i2c_driver.ko
```

---

# 18. Check Kernel Logs

```bash id="rjxmsr"
dmesg | tail
```

Expected:

```text id="tnqls2"
I2C Driver Probe Called
Register Value = 0xXX
```

---

# 19. Important I2C APIs

## i2c_smbus_read_byte_data()

Reads one register byte.

Example:

```c id="3j2d3f"
i2c_smbus_read_byte_data(client, reg);
```

---

## i2c_smbus_write_byte_data()

Writes one register byte.

Example:

```c id="h4bb7r"
i2c_smbus_write_byte_data(client, reg, value);
```

---

## i2c_master_send()

Sends raw data.

Example:

```c id="wjxy8i"
i2c_master_send(client, buffer, len);
```

---

## i2c_master_recv()

Receives raw data.

Example:

```c id="fhq3aq"
i2c_master_recv(client, buffer, len);
```

---

# 20. SMBus vs I2C

| Feature    | SMBus         | I2C           |
| ---------- | ------------- | ------------- |
| Complexity | Simple        | Flexible      |
| Speed      | Lower         | Higher        |
| Commands   | Standardized  | Generic       |
| Linux APIs | SMBus helpers | Raw transfers |

---

# 21. Advantages of I2C

| Advantage                   | Description                |
| --------------------------- | -------------------------- |
| Two-wire protocol           | Minimal wiring             |
| Multi-device support        | Many devices on same bus   |
| Address-based communication | Easy device selection      |
| Widely supported            | Common in embedded systems |
| Low cost                    | Simple hardware            |

---

# 22. Disadvantages of I2C

| Disadvantage               | Description                  |
| -------------------------- | ---------------------------- |
| Slow                       | Compared to SPI              |
| Short distance             | Not suitable for long cables |
| Limited bandwidth          | Low throughput               |
| Address conflicts          | Duplicate addresses possible |
| Pull-up resistors required | Extra hardware needed        |

---

# 23. I2C vs SPI

| Feature          | I2C      | SPI                    |
| ---------------- | -------- | ---------------------- |
| Wires            | 2        | 4+                     |
| Speed            | Moderate | High                   |
| Addressing       | Yes      | No                     |
| Complexity       | Simple   | Moderate               |
| Multiple Devices | Easy     | More chip-select lines |

---

# 24. Common Interview Questions

## Q1. What is I2C?

A two-wire serial communication protocol used for communication between processors and peripherals.

---

## Q2. Why Use I2C?

* Minimal wiring
* Multiple devices support
* Easy communication

---

## Q3. What is probe() in I2C Driver?

probe() is called when kernel matches I2C device with driver.

Used for:

* Device initialization
* Register configuration
* Resource allocation

---

## Q4. Difference Between I2C and SPI?

I2C uses addressing and two wires.

SPI uses separate chip select lines and is faster.

---

## Q5. What is i2c_client?

Represents an I2C slave device in Linux kernel.

Contains:

* Address
* Adapter
* Device information

---

# 25. Common Errors

## Error: I2C Read Failed

Cause:

* Wrong slave address
* Device not powered

Fix:

* Verify Device Tree
* Check hardware connection

---

## Error: No ACK Received

Cause:

* Incorrect address
* Bus issue

Fix:

* Verify SDA/SCL
* Check pull-up resistors

---

## Error: Device Not Detected

Cause:

* Driver mismatch

Fix:

* Verify compatible string

---

# 26. I2C Debugging Techniques

## Detect Devices

```bash id="v0e8a5"
i2cdetect -y 1
```

---

## Read Registers

```bash id="u5h79q"
i2cget -y 1 0x48 0x10
```

---

## Write Registers

```bash id="boqrx4"
i2cset -y 1 0x48 0x10 0x55
```

---

## Kernel Logs

```bash id="fd79q7"
dmesg | tail
```

---

# 27. Advanced I2C Topics

After learning basic I2C drivers, move to:

* Interrupt-driven I2C
* DMA support
* Multi-byte transfers
* EEPROM drivers
* PMIC drivers
* Touchscreen drivers
* I2C multiplexers
* Power management
* Regmap API

---

# 28. Best Practices

## Use SMBus Helpers

Preferred:

```c id="2pvb3v"
i2c_smbus_read_byte_data()
```

for simple register access.

---

## Always Check Return Values

```c id="6jz5x4"
if (ret < 0)
    return ret;
```

---

## Use Device Tree

Avoid hardcoded addresses.

---

## Handle Errors Properly

Bus failures are common in embedded systems.

---

# 29. Real Hardware Platforms

I2C drivers are widely used on:

* Raspberry Pi 5
* BeagleBone Black
* NVIDIA Jetson Nano
* STM32MP157

---

# 30. Popular I2C Devices

| Device       | Purpose                 |
| ------------ | ----------------------- |
| MPU6050      | Accelerometer/Gyroscope |
| DS1307       | RTC                     |
| AT24C256     | EEPROM                  |
| BMP280       | Pressure Sensor         |
| INA219       | Power Monitor           |
| OLED SSD1306 | Display                 |

---


```
```

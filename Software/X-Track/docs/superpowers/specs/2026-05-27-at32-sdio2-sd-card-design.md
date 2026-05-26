# AT32 SDIO2 SD Card Migration Design

## Goal

Replace the project's SPI-based SD card transport with the hardware SDIO2
interface so file reads, recording, and USB MSC access benefit from 4-bit
SDIO transfer while preserving the existing application-facing storage
behavior.

## Confirmed Hardware

The target controller is AT32F435/437 and the SD card is connected to SDIO2:

| Signal | Pin |
| --- | --- |
| Card detect | PA1 |
| SDIO2_CK | PA2 |
| SDIO2_CMD | PA3 |
| SDIO2_D0 | PA4 |
| SDIO2_D1 | PA5 |
| SDIO2_D2 | PA6 |
| SDIO2_D3 | PA7 |

The AT32F435/437 datasheet identifies these pins as valid SDIO2 alternate
functions. The implementation must configure the matching GPIO alternate
function selection documented in the AT32 reference manual.

## Existing Architecture

`USER/HAL/HAL_SD_CARD.cpp` owns the single mounted SD card instance. It:

- mounts a `SdFat` filesystem through SPI;
- reports card readiness, size, and type to the application;
- creates the track-record directory;
- implements USB MSC raw block read/write through `SD.card()`.

Application storage consumers use `SdFile` or the HAL status functions.
Consequently, LVGL filesystem integration, recorder code, image loading, and
USB MSC do not need separate interface rewrites when the block driver changes.

## Chosen Approach

Implement an AT32-specific SDIO block driver compatible with the existing
SdFat `BaseBlockDriver` contract, then mount the same Fat filesystem through
that driver in `HAL_SD_CARD.cpp`.

This avoids modifying all storage clients and avoids treating SdFat's existing
`SdFatSdio` implementation as portable: the bundled SDIO source is enabled
only for Teensy/Kinetis hardware and cannot directly drive AT32 SDIO2.

## Components

### Board Configuration

`USER/HAL/HAL_Config.h` will expose the confirmed SDIO2 pins and card-detect
pin. SPI-specific SD card bus and chip-select configuration will no longer be
used by the SD card HAL.

### AT32 SDIO Card Driver

A new driver in the project HAL layer will provide the operations required by
SdFat and USB MSC:

- initialization and card enumeration;
- card capacity and SD card type reporting;
- single- and multi-block reads;
- single- and multi-block writes;
- synchronization and error reporting.

The driver will use the existing AT32 standard peripheral SDIO APIs and a free
DMA channel routed with `DMAMUX_DMAREQ_ID_SDIO2`. Existing code currently uses
`DMA1_CHANNEL1` for ADC and `DMA1_CHANNEL3` for display transmission, so the
SDIO assignment must not conflict with either channel.

### Filesystem Adapter

`HAL_SD_CARD.cpp` will replace the SPI-backed `SdFat` object with a filesystem
object templated on the AT32 SDIO driver. Its public HAL behavior remains
unchanged:

- card insertion/removal callbacks remain intact;
- size and type reporting continue through the block driver;
- track directory creation remains after successful mount;
- USB MSC capacity/read/write functions continue to access raw blocks through
  the mounted card object.

## Initialization And Data Flow

On insertion, the HAL keeps the existing card-detect check, then initializes
SDIO2 in native SD mode:

1. Configure SDIO2 GPIOs, peripheral clock, power, and a low initialization
   bus clock in 1-bit mode.
2. Enumerate the card using the native SD command sequence, including reset,
   voltage/capability negotiation, RCA selection, and card selection.
3. Read card metadata needed for capacity and type.
4. Request 4-bit bus mode, configure SDIO2 for 4-bit operation, and raise the
   transfer clock to the configured operational rate.
5. Mount the Fat filesystem through the SDIO block driver.

Normal file operations then flow from existing `SdFile` clients through
SdFat/FatLib to the new block driver. USB MSC calls bypass file operations but
use the same SDIO driver's raw block methods.

## Transfer Strategy

The target implementation is 4-bit SDIO2 with DMA-backed block transfer and
multi-block operations where the SdFat request contains more than one sector.
Single-sector operations remain supported for filesystem metadata and small
accesses. The driver must stop or synchronize active multi-block writes before
the card is exposed as complete to FatLib or USB MSC.

## Error Handling

- Initialization returns failure if card detection, command negotiation,
  4-bit mode selection, or filesystem mounting fails.
- SDIO command timeout, response CRC, data CRC, overrun, underrun, and transfer
  timeout conditions are converted to block-driver failure/error status.
- A failed read or write propagates through existing USB MSC return values and
  filesystem operations.
- On removal, readiness and cached capacity are cleared as they are today.

## Verification

Static verification will include a firmware build after integration and checks
that SPI SD initialization is no longer reachable in the SD HAL.

Hardware verification will cover:

- insert/remove detection and successful mount;
- reported card type and capacity;
- create/write/read a recorded track file;
- LVGL/image reads from the card;
- USB MSC host read and write;
- repeated multi-block large-file read/write with throughput comparison against
  the previous SPI implementation.

## Scope

This change covers the hardware storage transport and the existing storage
surfaces that already route through `HAL_SD_CARD.cpp`. It does not change
unrelated display SPI usage, filesystem format, application file formats, or
USB class behavior beyond its underlying media transport.

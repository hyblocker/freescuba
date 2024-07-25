# FreeScuba
> Work in progress low latency open source ContactGlove drivers, with backwards finger bending.

## Demo

https://github.com/user-attachments/assets/fe962642-e5fb-4a0d-80cd-b1cabb9cb9e2

![Overlay user interface](https://github.com/hyblocker/freescuba/assets/7695629/944e0213-6acd-4c5c-8939-0d08c8547449)

## What is this?

This project is a custom, open source driver for Diver-X ContactGloves. It aims to allow the hardware to reach it's full potential by attempting to eliminate all software limitations the official drivers have.

This driver has a few features which the official drivers do not have:
- Low latency SteamVR driver
- Support for backwards finger bending

## How do I get this?
Compile it. The drivers are currently not stable enough for me to consider distributing an installer, and as such these is currently primarily aimed at tinkerers who would like to provide feedback / PR to see the driver improve.

## Status

### Hardware Protocol

| Feature         | Reverse Engineered |
| ------------------------ | --------- |
| Receiving battery status | ✅        |
| Magnetra support         | ✅        |
| Finger tracking          | ✅        |
| Haptics                  | ❌        |

### Driver Features

| Feature | Status |
| -------- | --------- |
| Pose calibration  | "Works" but needs more refinement, as it's difficult to fine tune the calibration |
| Finger tracking calibration    | "Works" but needs more refinement, as it's difficult to consistently get a good pose where your fingers match what they are doing in real life |
| Joystick calibration | Effectively done |
| Manual pose offsets | Partially implemented |
| Skeletal input | "Works" but minimally tested + needs a lot of fine tuning |
| "Weirdness" | Sometimes the gloves' hardware lies, but this driver does not handle that well at present. |
| Pretty UI | TBA |
| Installer | TBA |

### OS Support

| Platform | Supported |
| -------- | --------- |
| Windows  | ✅         |
| MacOS    | ❌         |
| Linux    | ❌         |

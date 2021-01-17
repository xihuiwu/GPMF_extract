# GPMF_extract
This repository uses API of [GPMF Parser](https://github.com/gopro/gpmf-parser) to extract sensor information from MP4 file. Notice that MP4 file must be generated from GoPro cameras. Current supported sensor type are accelerometer, gyroscope, and GPS.

## Supported Sensor Type
* Accelerometer
* Gyroscope
* GPS

## Compile
Run the shell script\
`./compile.sh`

## Use
`./extract $file_dir$`

## Utility
* low pass filter
* data extraction from csv file

## Future Development
- [x] Add low-pass filter
- [ ] Extract sensor data that are linked to every frame

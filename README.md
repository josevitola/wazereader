# WazeReader

 Fetch the coordinates of events (accidents, traffic jams, closures, etc...) displayed on a Waze map for Bogotá, Colombia.

## Getting Started

This program is specifically designed for a 1920 x 1080 screen running a 100%-zoom Google Chrome window with a 16z map zoom. Currently the program recognizes:
* accidents
* traffic jams (moderate, heavy, standstill)
* police
* hazards
* closures

For a simpler version you can also check out [MapReader](https://github.com/jdnietov/mapreader), which uses Google Maps snapshots and only fetches accident icons.

### Prerequisites

- Bash script interpreter
- G++ compiler
- Google Chrome +59.0 (uses `--headless` flag)
- OpenCV 2.4 (you can follow the [official instructions](http://docs.opencv.org/2.4/doc/tutorials/introduction/linux_install/linux_install.html) -- or use [these](https://github.com/jayrambhia/Install-OpenCV) handy scripts)

## Installing

Clone the repository in your machine and run

```
cd wazeReader/
make
```

## Usage

```
./wazeReader [--graphic]
```

All results will be written to a `data.log` file.
- You can add a `--graphic` flag to display results on a window.

## Authors

* **José David Nieto Vitola** - *Initial work* - [jdnietov](https://github.com/jdnietov)

## License

This project is licensed under the MIT License.

## Acknowledgments

* This project is based on [MapReader](https://github.com/jdnietov/mapreader) -- a Google Maps-based previous version, which only reads accidents on the map (no, I didn't know what forking was when I wrote these).

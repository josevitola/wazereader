"""This module takes the data.log file produced by main.cpp and
fetches Bogota's addresses based on the coordinates in the file.

TODO: check if system has requirements - if not, install them
* requests
* subprocess (upcoming)
TODO: include exact time of match
TODO: progress bar

FIXME: addresses are too vague. select best one
"""

import requests

GOOGLE_MAPS_API_URL = 'http://maps.googleapis.com/maps/api/geocode/json'

LOGNAME = "coordinate.log"
DATANAME = "data.log"

def main():
    """Main function. Read coordinates, fetch addresses and write on file."""

    logfile = open(LOGNAME, "r")
    datafile = open(DATANAME, "w")

    logfile.readline()   # first line is always a date
    print("fetching addresses...")

    line = logfile.readline()
    while not line.startswith("***") and line.strip():
        cat, lat, lng = line.split(';')

        latlng = "%s,%s" % (lat, lng)
        params = {
            'latlng': latlng
        }

        req = requests.get(GOOGLE_MAPS_API_URL, params=params)
        res = req.json()
        result = res['results'][0]
        address = result['formatted_address']

        datafile.write("%s en %s |%s,%s" % (cat, address.partition(",")[0], lat, lng))

        line = logfile.readline()

    logfile.close()
    datafile.close()

    print("done.")

main()

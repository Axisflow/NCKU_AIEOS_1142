loop 0
    read TEMP /dev/temp0
    read HUM /dev/hum0

    print TEMP
    print HUM

    sleep 2000
endloop
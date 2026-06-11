loop 0
    read TEMP /dev/temp1
    read HUM /dev/hum1

    print TEMP
    print HUM

    if HUM >= 70
        echo 1 > /dev/RedLED
        echo 0 > /dev/GreenLED
    else
        echo 0 > /dev/RedLED
        echo 1 > /dev/GreenLED
    endif

    sleep 2000
endloop
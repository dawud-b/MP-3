#!/bin/sh


LEDS_ADDR=0x41200000
BUTTON_ADDR=0x41210000
SWITCH_ADDR=0x41220000

while true; do
	num_btns=0
	BUTTONS=$(devmem $BUTTON_ADDR)
	temp=$((BUTTONS & 0x1F))
	while [ $temp -gt 0 ]; do
		num_btns=$((num_btns + (temp & 1)))
		temp=$((temp >> 1))
	done

	SWITCHES=$(devmem $SWITCH_ADDR)
	SWITCHES=$((SWITCHES & 0xFFF))

	if [ $num_btns -eq 0 ]; then
		# LED = switches
		devmem $LEDS_ADDR 32 $SWITCHES
	elif [ $num_btns -eq 1 ]; then
		# LED = inverse switches
		inv_sw=$(((~SWITCHES) & 0xFFF ))
		devmem $LEDS_ADDR 32 $inv_sw
	else
		# LED = all 1s
		devmem $LEDS_ADDR 32 0xFFF
	fi

done


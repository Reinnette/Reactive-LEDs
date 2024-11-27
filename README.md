<h1>Overview</h1>

This was a fun project I did with a friend of mine in our free time to use in cosplay. 
We tag teamed where I handled all of the software and they did the hardware.

The Project uses the FastLED Library as well as the I2S protocol for the option microphone.

Using the Config options at the top of the file as well as the items required within Setup() this can be configured to run with as many led strips as is wanted and the pattern will be mirrored on each strip.
Currently it does this by going setting the value of each led in a strip in the sequence before moving onto the next strip. This worked fine for 6 led strips each having 144 leds.

<h1>Modes</h1>

Eight different Modes are currently programmed 
Static: A single static color.
Alternating Colors: Ever other led will be of a set 2 colors
Swapping Colors: Will slowly change each led to one color then on the next pass through swaps to a different color
Sound Reactive: Uses an I2S Microphone to read in sound from around itself. Then uses three of the Frequency Bandwiths to display a color based on the average of the bandwith.
Breathing Effect: Uses a timmer to slowly turn on to one color then to turn off.
Stripes: Will segment the led strips into 5 parts to then color each region with a set color.
Center Out Effect: Changes the color of the leds from the center outwards.
Out Center Effect: Changes the color of the leds from the edges inwards.

<h1>Preview</h1>

https://github.com/user-attachments/assets/92430642-0bc8-4d9b-8e9b-8a7f05c18dfc

https://github.com/user-attachments/assets/c81730fb-a752-48e2-8e2b-05a2e7ce51f9


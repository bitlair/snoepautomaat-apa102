#include <SPI.h>

// MOSI = 11 (red)
// SCK  = 13 (green)

const unsigned int NUM_LEDS = 70;

const char gs = 15;
const SPISettings spiSettings(1000000, MSBFIRST, SPI_MODE3);

void setup() {
    srand(0);
    Serial.begin(9600);
    SPI.begin();
}

void hsv2rgb(float h, float s, float v, float *r, float *g, float *b) {
    if (s <= 0.0) {
        *r = v;
        *g = v;
        *b = v;
        return;
    }
    float hh = h;
    if (hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    int i = (int)hh;
    float ff = hh - i;
    float p = v * (1.0 - s);
    float q = v * (1.0 - (s * ff));
    float t = v * (1.0 - (s * (1.0 - ff)));

    switch(i) {
    case 0:
        *r = v;
        *g = t;
        *b = p;
        break;
    case 1:
        *r = q;
        *g = v;
        *b = p;
        break;
    case 2:
        *r = p;
        *g = v;
        *b = t;
        break;

    case 3:
        *r = p;
        *g = q;
        *b = v;
        break;
    case 4:
        *r = t;
        *g = p;
        *b = v;
        break;
    case 5:
    default:
        *r = v;
        *g = p;
        *b = q;
        break;
    }
}


void calc_frame(unsigned char *frame, unsigned int num_leds) {
    for (int i = 0; i < num_leds * 3; i += 3) {
        float r, g, b;
        float h = (float)((i + millis() / 100) % num_leds * 3) / (float)(num_leds * 3);
        hsv2rgb(h * 360, 1, 1, &r, &g, &b);
        frame[i+0] = (char)(r * 255);
        frame[i+1] = (char)(g * 255);
        frame[i+2] = (char)(b * 255);
    }
}

// Christmas!
void calc_frame_christmas(unsigned char *frame, unsigned int num_leds) {
    const int num_fading_leds = 40;

    static struct fading_leds {
        unsigned int index;
        unsigned char value;
        unsigned char speed;
    } fading_leds[num_fading_leds] = { 0 };

    static bool seed = true;
    if (seed) {
        seed = false;
        for (int f = 0; f < num_fading_leds; f++) {
            fading_leds[f] = {
                .index = rand() % num_leds,
                .value = 0xff,
                .speed = 1,
            };
        }
    }

    for (int i = 0; i < num_leds * 3; i += 3) {
        int fade_index = -1;
        for (int f = 0; f < num_fading_leds; f++) {
            if (i == fading_leds[f].index * 3) {
                fade_index = f;
                break;
            }
        }

        if (fade_index >= 0) {
            unsigned char *val = &fading_leds[fade_index].value;
            frame[i+0] = *val;
            frame[i+1] = 0xff - *val;
            frame[i+2] = 0x00;

            *val -= fading_leds[fade_index].speed;
            if (*val < 32) {
                fading_leds[fade_index] = {
                    .index = rand() % num_leds,
                    .value = 0xff,
                    .speed = 1 + rand() % 4,
                };
            }
        } else {
            frame[i+0] = 0x00;
            frame[i+1] = 0xff;
            frame[i+2] = 0x00;
        }
    }
}

void loop() {
    static unsigned char framebuf[NUM_LEDS * 3] = { 0 };

    calc_frame(framebuf, NUM_LEDS);

    SPI.beginTransaction(spiSettings);
    // Start frame
    SPI.transfer(0x00);
    SPI.transfer(0x00);
    SPI.transfer(0x00);
    SPI.transfer(0x00);

    // LEDs
    for (int i = 0; i < NUM_LEDS * 3; i += 3) {
        SPI.transfer(0b11100000 | gs);
        SPI.transfer(framebuf[i + 2]);  // blue
        SPI.transfer(framebuf[i + 1]);  // green
        SPI.transfer(framebuf[i + 0]);  // red
    }

    SPI.endTransaction();
}

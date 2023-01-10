#define ENCODER_DATA 17
#define ENCODER_CLK 13
#define FAN_TACH 33
#define FAN_PWM 32
#define MAX_RPM NAN
#include <ttgo.hpp>
#include <encoder.hpp>
#include <fan_controller.hpp>
// downloaded from fontsquirrel.com and header generated with 
// https://honeythecodewitch.com/gfx/generator
#include <fonts/Telegrama.hpp>
static const open_font& text_font = Telegrama;

// hardware external to the TTGO
static int_encoder<ENCODER_DATA,ENCODER_CLK,true> knob;
static fan_controller fan(
    [](uint16_t duty,void* state){ ledcWrite(0,duty>>8); },
    nullptr,
    FAN_TACH, MAX_RPM);

// the frame buffer used for double buffering
using frame_buffer_t = bitmap<typename lcd_t::pixel_type>;
static uint8_t frame_buffer_data[frame_buffer_t::sizeof_buffer({lcd_t::base_width,lcd_t::base_height})];

// temporary strings for formatting
static char tmpsz1[256];
static char tmpsz2[256];
static char tmpsz3[256];

static int mode = 0; // 0 = RPM, 1 = PWM

// bookkeeping cruft
static float old_rpm=NAN;
static long long old_knob=-1;
static uint32_t ts=0;
float new_rpm = 0;

// draw text in center of screen
static void draw_center_text(const char* text, int size=30) {
    // finish any pending async draws
    draw::wait_all_async(lcd);

    // get a bitmap over our frame buffer
    frame_buffer_t frame_buffer(lcd.dimensions(),frame_buffer_data);
    // clear it to purple
    draw::filled_rectangle(frame_buffer,frame_buffer.bounds(),color_t::purple);

    // fill the text structure
    open_text_info oti;
    oti.font = &text_font;
    oti.text = text;
    // scale the font to the right line height
    oti.scale = oti.font->scale(size);
    // measure the text
    srect16 txtr = oti.font->measure_text(ssize16::max(),spoint16::zero(),oti.text,oti.scale).bounds();
    // center what we got back
    txtr.center_inplace((srect16)frame_buffer.bounds());
    // draw it to the frame buffer
    draw::text(frame_buffer,txtr,oti,color_t::white,color_t::purple);

    // asynchronously send the frame buffer to the LCD (uses DMA)
    draw::bitmap_async(lcd,lcd.bounds(),frame_buffer,frame_buffer.bounds());
}
// draw centered text in more than one area
static void draw_center_status_text(const char* text1, const char* text2, const char* text3, int size=30) {
    // finish any pending async draws
    draw::wait_all_async(lcd);

    // get a bitmap over our frame buffer
    frame_buffer_t frame_buffer(lcd.dimensions(),frame_buffer_data);
    // clear it to purple
    draw::filled_rectangle(frame_buffer,frame_buffer.bounds(),color_t::purple);

    // fill the text structure
    open_text_info oti;
    oti.font = &text_font;
    oti.text = text1;
    // scale the font to the right line height
    oti.scale = oti.font->scale(size);
    // measure the text
    srect16 txtr = oti.font->measure_text(ssize16::max(),spoint16::zero(),oti.text,oti.scale).bounds();
    // center what we got back horizontally
    txtr.center_horizontal_inplace((srect16)frame_buffer.bounds());
    // move it down 10 pixels
    txtr.offset_inplace(0,10);
    // draw it
    draw::text(frame_buffer,txtr,oti,color_t::white,color_t::purple);
    
    // set the next text
    oti.text = text2;
    // measure it
    txtr = oti.font->measure_text(ssize16::max(),spoint16::zero(),oti.text,oti.scale).bounds();
    // center it horizontally
    txtr.center_horizontal_inplace((srect16)frame_buffer.bounds());
    // offset 10 pixels from the bottom of the previous text
    txtr.offset_inplace(0,size+20);
    // draw it
    draw::text(frame_buffer,txtr,oti,color_t::white,color_t::purple);

    // set the final text
    oti.text = text3;
    // measure it
    txtr = oti.font->measure_text(ssize16::max(),spoint16::zero(),oti.text,oti.scale).bounds();
    // center it horizontally
    txtr.center_horizontal_inplace((srect16)frame_buffer.bounds());
    // offset 10 pixels from the bottom of the screen
    txtr.offset_inplace(0,frame_buffer.dimensions().height-size-10);
    // draw the text to the frame buffer
    draw::text(frame_buffer,txtr,oti,color_t::white,color_t::purple);

    // asynchronously send it to the LCD
    draw::bitmap_async(lcd,lcd.bounds(),frame_buffer,frame_buffer.bounds());
}
// for the button
static void on_click_handler(int clicks, void* state) {
    // reduce the clicks to odd or even and set the mode accordingly
    mode = (mode+(clicks&1))&1;
    // reset the timestamp for immediate update
    ts = 0;
    if(mode==0) {
        // set the new RPM from the current RPM
        new_rpm = fan.rpm();
        if(new_rpm>fan.max_rpm()) {
            new_rpm = fan.max_rpm();
        }
        // set the knob's position to reflect it
        knob.position(((float)new_rpm/fan.max_rpm())*100);
    } else {
        new_rpm = NAN;
    }
    // force the loop to reconsider the knob position
    --old_knob;
    old_rpm = NAN; // forces a redraw
}
void setup() {
    Serial.begin(115200);
    // init the ttgo
    ttgo_initialize();
    // landscape mode, buttons on right
    lcd.rotation(1);
    // set up the PWM generator to 25KHz, channel 0, 8-bit
    ledcSetup(0,25*1000,8);
    ledcAttachPin(FAN_PWM,0);

    // if indicated, fan.initialize() will detect the max RPM, so we 
    // display a message indicating that beforehand
    if(MAX_RPM!=MAX_RPM) {
        draw_center_text("detecting fan...",20);
    }
    // init the fan
    fan.initialize();
    // turn the fan off
    fan.pwm_duty(0);
    // display the max RPM
    snprintf(tmpsz1,
            sizeof(tmpsz1),
            "Max RPM: %d",
            (int)fan.max_rpm());
    draw_center_text(tmpsz1,25);
    // init the encoder knob
    knob.initialize();
    // set the button callbacks (already initialized via ttgo_initialize())
    button_a.on_click(on_click_handler);
    button_b.on_click(on_click_handler);
    // delay 3 seconds
    delay(3000);
}
void loop() {
    // give the fan a chance to process
    fan.update();
    // range limit the knob to 0-100, inclusive
    if(knob.position()<0) {
        knob.position(0);
    } else if(knob.position()>100) {
        knob.position(100);
    }
    // trivial timer
    uint32_t ms = millis();
    // every quarter second...
    if(ms>ts+250) {
        ts = ms;
        // if RPM changed
        if(old_rpm!=fan.rpm()) {
            // print the info
            Serial.print("RPM: ");
            Serial.println(fan.rpm());
            old_rpm = fan.rpm();
            // format the RPM
            snprintf(tmpsz1,
                    sizeof(tmpsz1),
                    "Fan RPM: %d",
                    (int)fan.rpm());
            Serial.print("RPM: ");
            Serial.println(fan.rpm());
            old_rpm = fan.rpm();
            // format the PWM
            snprintf(tmpsz2,
                    sizeof(tmpsz2),
                    "Fan PWM: %d%%",
                    (int)(((float)fan.pwm_duty()/65535.0)*100.0));
            if(mode==0) {
                // format the target RPM
                snprintf(tmpsz3,
                        sizeof(tmpsz3),
                        "Set RPM: %d",
                        (int)new_rpm);
            } else {
                // format the target PWM
                snprintf(tmpsz3,
                        sizeof(tmpsz3),
                        "Set PWM: %d%%",(int)knob.position());
            }
            draw_center_status_text(tmpsz1,tmpsz2,tmpsz3,25);
            
        }
    }
    // if the knob position changed   
    if(old_knob!=knob.position()) {
        Serial.print("Knob: ");
        Serial.println(knob.position());
        if(mode==0) {
            // set the new RPM
            new_rpm = fan.max_rpm()*
                (knob.position()/100.0);
            Serial.print("New RPM: ");
            Serial.println(new_rpm);
            fan.rpm(new_rpm);
        } else {
            // set the new PWM
            new_rpm = NAN;
            fan.pwm_duty(65535.0*(knob.position()/100.0));
        }
        // force redraw:
        old_rpm = NAN;
        old_knob = knob.position();
    }
    // we don't use the dimmer, so make sure it doesn't timeout
    dimmer.wake();
    // give the TTGO hardware a chance to process
    ttgo_update();
}
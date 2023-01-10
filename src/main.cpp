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

static int_encoder<ENCODER_DATA,ENCODER_CLK,true> knob;
static fan_controller fan(
    [](uint16_t duty,void* state){ ledcWrite(0,duty>>8); },
    nullptr,
    FAN_TACH, MAX_RPM);

static char tmpsz1[256];
static char tmpsz2[256];

static int mode = 0; // 0 = RPM, 1 = PWM

static float old_rpm=NAN;
static long long old_knob=-1;
static uint32_t ts=0;
float new_rpm = 0;

static void draw_center_text(const char* text, int size=30) {
    draw::filled_rectangle(lcd,lcd.bounds(),color_t::purple);
    open_text_info oti;
    oti.font = &Telegrama;
    oti.text = text;
    oti.scale = oti.font->scale(size);
    oti.transparent_background = false;
    srect16 txtr = oti.font->measure_text(ssize16::max(),spoint16::zero(),oti.text,oti.scale).bounds();
    txtr.center_inplace((srect16)lcd.bounds());
    draw::text(lcd,txtr,oti,color_t::white,color_t::purple);
}
static void draw_center_2_text(const char* text1, const char* text2, int size=30) {
    draw::filled_rectangle(lcd,lcd.bounds(),color_t::purple);
    open_text_info oti;
    oti.font = &Telegrama;
    oti.text = text1;
    oti.scale = oti.font->scale(size);
    oti.transparent_background = false;
    srect16 txtr = oti.font->measure_text(ssize16::max(),spoint16::zero(),oti.text,oti.scale).bounds();
    txtr.center_horizontal_inplace((srect16)lcd.bounds());
    txtr.offset_inplace(0,10);
    draw::text(lcd,txtr,oti,color_t::white,color_t::purple);
    oti.text = text2;
    txtr = oti.font->measure_text(ssize16::max(),spoint16::zero(),oti.text,oti.scale).bounds();
    txtr.center_horizontal_inplace((srect16)lcd.bounds());
    txtr.offset_inplace(0,lcd.dimensions().height-size-10);
    draw::text(lcd,txtr,oti,color_t::white,color_t::purple);
}
static void on_click_handler(int clicks, void* state) {
    mode = (mode+(clicks&1))&1;
    ts = 0;
    if(mode==0) {
        new_rpm = fan.rpm();
        if(new_rpm>fan.max_rpm()) {
            new_rpm = fan.max_rpm();
        }
        knob.position(((float)new_rpm/fan.max_rpm())*100);
    }
    old_rpm = NAN; // forces a redraw
}
void setup() {
    Serial.begin(115200);
    ttgo_initialize();
    lcd.rotation(1);
    
    ledcSetup(0,25*1000,8);
    ledcAttachPin(FAN_PWM,0);
    if(MAX_RPM!=MAX_RPM) {
        draw_center_text("detecting fan...",20);
    }
    fan.initialize();
    fan.pwm_duty(0);
    snprintf(tmpsz1,
            sizeof(tmpsz1),
            "Max RPM: %d",
            (int)fan.max_rpm());
    draw_center_text(tmpsz1,20);
    knob.initialize();
    button_a.on_click(on_click_handler);
    button_b.on_click(on_click_handler);
    delay(3000);
}
void loop() {
    fan.update();
    if(knob.position()<0) {
        knob.position(0);
    } else if(knob.position()>100) {
        knob.position(100);
    }
    uint32_t ms = millis();
    if(ms>ts+250) {
        ts = ms;
        if(old_rpm!=fan.rpm()) {
            Serial.print("RPM: ");
            Serial.println(fan.rpm());
            old_rpm = fan.rpm();
            snprintf(tmpsz1,
                    sizeof(tmpsz1),
                    "Fan RPM: %d",
                    (int)fan.rpm());
            Serial.print("RPM: ");
            Serial.println(fan.rpm());
            old_rpm = fan.rpm();
            if(mode==0) {
                snprintf(tmpsz2,
                        sizeof(tmpsz2),
                        "Set RPM: %d",
                        (int)new_rpm);
            } else {
                snprintf(tmpsz2,
                        sizeof(tmpsz2),
                        "Set PWM: %d%%",(int)knob.position());
            }
            draw_center_2_text(tmpsz1,tmpsz2,20);
            
        }
    }
    
    if(old_knob!=knob.position()) {
        Serial.print("Knob: ");
        Serial.println(knob.position());
        if(mode==0) {
            new_rpm = fan.max_rpm()*
                (knob.position()/100.0);
            Serial.print("New RPM: ");
            Serial.println(new_rpm);
            fan.rpm(new_rpm);
        } else {
            new_rpm = NAN;
            fan.pwm_duty(65535.0*(knob.position()/100.0));
        }
        // force redraw:
        old_rpm = NAN;
        old_knob = knob.position();
    }

    dimmer.wake();
    ttgo_update();
}
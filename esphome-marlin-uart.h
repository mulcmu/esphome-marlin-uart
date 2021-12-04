
//class constructor based off of @mannkind
//ESPHomeRoombaComponent/ESPHomeRoombaComponent.h
//https://github.com/mannkind/ESPHomeRoombaComponent

#include "esphome.h"

#define PROGRESS_INTERVAL 15000 //15 seconds

static const char *TAG = "component.MarlinUART";

class component_MarlinUART : 
        public PollingComponent,
        public UARTDevice,
        public CustomAPIDevice {

  public:
    Sensor *sensor_bedtemp;
    Sensor *sensor_bedsetpoint;
    
    Sensor *sensor_exttemp;
    Sensor *sensor_extsetpoint;
    
    Sensor *sensor_progress;
    
    TextSensor *textsensor_printerState;

    static component_MarlinUART* instance(UARTComponent *parent)
    {
        static component_MarlinUART* INSTANCE = new component_MarlinUART(parent);
        return INSTANCE;
    }

    //delay setup() so that printer has booted before sending the M155 command.
    float get_setup_priority() const override { 
        return esphome::setup_priority::LATE; 
    }

    void setup() override
    {
        ESP_LOGD(TAG, "setup().");
        
        MarlinOutput.reserve(256);  //allocate hefty buffer for String object
        MarlinOutput = "";
        
        flush();
        write_str("\r\n\r\nM155 S10\r\n");  //Auto report temperatures every 10 seconds
        
        textsensor_printerState->publish_state("Unknown");
        sensor_progress->publish_state(NAN);
        
        //Allow home assistant to preheat the printer
        register_service(&component_MarlinUART::set_bed_setpoint, "set_bed_setpoint",
                 {"temp_degC"});
        register_service(&component_MarlinUART::set_extruder_setpoint, "set_extruder_setpoint",
                {"temp_degC"});       
    }

    void update() override
    {
        ESP_LOGV(TAG, "update().");

        int16_t distance;
        uint16_t voltage;
        bool publishJson;
        
        while ( available() ) {
            char c = read();
            MarlinOutput += c;
            if( c == '\n' || c == '\r' ) {
                process_line();
            }
        }
        
        
        if(millis() - millisProgress > PROGRESS_INTERVAL)  {
            write_str("M27\r\n");
            millisProgress = millis();
        }
        // Only publish new states if there was a change

        /*
        if (this->cleaningBinarySensor->state != cleaningState) {
            this->cleaningBinarySensor->publish_state(cleaningState);
        } 
        */
    }

    //TODO define range
    void set_bed_setpoint(int temp_degC) {
        ESP_LOGD(TAG, "set_bed_setpoint().");
        if(temp_degC <0)
            return;
        if(temp_degC > 80)
            return;
        
        char buf[16];
        std::sprintf(buf, "M140 S%d\r\n", temp_degC);
        write_str(buf);
        ESP_LOGD(TAG, buf);
    }

    //TODO define range
    void set_extruder_setpoint(int temp_degC) {
        ESP_LOGD(TAG, "set_extruder_setpoint().");
        if(temp_degC <0)
            return;
        if(temp_degC > 240)
            return;
        
        char buf[16];
        std::sprintf(buf, "M104 S%d\r\n", temp_degC);
        write_str(buf);
        ESP_LOGD(TAG, buf);
    }
        
  private: 
    String MarlinOutput;
    String StateText;
    unsigned long millisProgress=0;
    

    component_MarlinUART(UARTComponent *parent) : PollingComponent(2000), UARTDevice(parent) 
    {
       
        this->sensor_bedtemp = new Sensor();
        this->sensor_bedsetpoint = new Sensor();
        
        this->sensor_exttemp = new Sensor();
        this->sensor_extsetpoint = new Sensor();
        
        this->sensor_progress = new Sensor();
    
        this->textsensor_printerState = new TextSensor();
    }
    
  
    void process_line() {
        
        ESP_LOGD(TAG, MarlinOutput.c_str() );
        
        // Auto reported temperature
        // T:157.35 /0.00 B:56.56 /0.00 @:0 B@:0
        // T:19.57 /0.00 B:20.12 /0.00 @:0 B@:
        if(MarlinOutput.startsWith(String(" T:"))) {

            //Extruder temp
            short start=3;
            short end = MarlinOutput.indexOf('/');
            if (end == -1) {
                MarlinOutput="";
                return;
            }
                
            float t = MarlinOutput.substring(start,end).toFloat();
            sensor_exttemp->publish_state(t);

            //Extruder setpoint
            start = end + 1;
            end = MarlinOutput.indexOf(' ', start);
            if (end == -1) {
                MarlinOutput="";
                return;
            }
            t = MarlinOutput.substring(start,end).toFloat();
            sensor_extsetpoint->publish_state(t);
            
            //Bed temp
            start = end + 3;
            end = MarlinOutput.indexOf('/', start);
            if (end == -1) {
                MarlinOutput="";
                return;
            }
            t = MarlinOutput.substring(start,end).toFloat();
            sensor_bedtemp->publish_state(t);
            
            //Bed setpoint
            start = end + 1;
            end = MarlinOutput.indexOf(' ', start);
            if (end == -1) {
                MarlinOutput="";
                return;
            }
            t = MarlinOutput.substring(start,end).toFloat();
            sensor_bedsetpoint->publish_state(t);
            
            MarlinOutput="";
            return;
        }
        
        //SD printing byte 2467546/3281364
       if(MarlinOutput.startsWith(String("SD printing byte "))) {
            long current, total;
            
            current = MarlinOutput.substring(17).toInt();
            total = MarlinOutput.substring(MarlinOutput.indexOf('/')+1).toInt();
            
            ESP_LOGD(TAG,String(current).c_str());
            ESP_LOGD(TAG,String(total).c_str());
            
            if (total==0)  {
                sensor_progress->publish_state(NAN);
                        }
            else  {
                sensor_progress->publish_state( (float) current / (float) total * 100.0);
            }
            
            MarlinOutput="";
            return;
        } 
        
        
        //State
        //action:prompt_begin FilamentRunout T0
        if(MarlinOutput.indexOf(String("FilamentRunout")) != -1) {
            textsensor_printerState->publish_state("Filament Runout");
            MarlinOutput="";
            return;
        }
        
        if(MarlinOutput.startsWith(String("echo:busy: paused"))) {
            if(textsensor_printerState->state != "Filament Runout") {
                textsensor_printerState->publish_state("Printing Paused");
            }
            MarlinOutput="";
            return;
        }
                
        if(MarlinOutput.startsWith(String("Done printing"))) {
            textsensor_printerState->publish_state("Finished Printing");
            sensor_progress->publish_state(NAN);
            MarlinOutput="";
            return;
        }
        
        if(MarlinOutput.startsWith(String("echo:busy: processing"))) {
            textsensor_printerState->publish_state("Printing");
            MarlinOutput="";
            return;
        }
        
        
        //reset string for next line
        MarlinOutput="";
    }
};
    

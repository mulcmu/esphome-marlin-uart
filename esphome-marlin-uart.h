
//class constructor based off of @mannkind
//ESPHomeRoombaComponent/ESPHomeRoombaComponent.h
//https://github.com/mannkind/ESPHomeRoombaComponent

#include "esphome.h"

//ComponentMarlinUart

#define CMU_PROGRESS_INTERVAL 15000 //15 seconds
#define CMU_BED_MAX_TEMP 80
#define CMU_EXT_MAX_TEMP 240

#define CMU_STATE_UNKNOWN 0
#define CMU_STATE_IDLE 1
#define CMU_STATE_PREHEAT 2
#define CMU_STATE_PRINTING 3
#define CMU_STATE_RUNOUT 4
#define CMU_STATE_PAUSED 5
#define CMU_STATE_ABORT 6
#define CMU_STATE_FINISHED 7
#define CMU_STATE_HALTED 8
#define CMU_STATE_COOLING 9

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
    TextSensor *textsensor_elapsedTime;
    TextSensor *textsensor_remainingTime;
    
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
        
        MarlinOutput.reserve(256);  //allocate hefty buffers for String objects
        MarlinOutput = "";
        StateText.reserve(32);
        S_time.reserve(32);
        
        flush();
        write_str("\r\n\r\nM155 S10\r\n");  //Gcode command to auto report temperatures every 10 seconds
        
        set_state(CMU_STATE_UNKNOWN);
        
        //Services to allow home assistant to preheat the printer
        register_service(&component_MarlinUART::set_bed_setpoint, "set_bed_setpoint",
                 {"temp_degC"});
        register_service(&component_MarlinUART::set_extruder_setpoint, "set_extruder_setpoint",
                {"temp_degC"});       
    }

    void update() override
    {
        ESP_LOGV(TAG, "update().");

        if (state == CMU_STATE_HALTED) 
            return;
        
        while ( available() ) {
            char c = read();
            MarlinOutput += c;
            if( c == '\n' || c == '\r' ) {
                process_line();
            }
        }
       
        if(millis() - millisProgress > CMU_PROGRESS_INTERVAL)  {
            millisProgress = millis();
            if(state==CMU_STATE_PRINTING)
                write_str("M27\r\nM31\r\n");  //Gcode to get remaining time and file progress
        }

    }

    void set_bed_setpoint(int temp_degC) {
        ESP_LOGD(TAG, "set_bed_setpoint().");
        if(temp_degC <0)
            return;
        if(temp_degC > CMU_BED_MAX_TEMP)
            return;
        
        char buf[16];
        std::sprintf(buf, "M140 S%d\r\n", temp_degC);
        write_str(buf);
        ESP_LOGD(TAG, buf);
    }

    void set_extruder_setpoint(int temp_degC) {
        ESP_LOGD(TAG, "set_extruder_setpoint().");
        if(temp_degC <0)
            return;
        if(temp_degC > CMU_EXT_MAX_TEMP)
            return;
        
        char buf[16];
        std::sprintf(buf, "M104 S%d\r\n", temp_degC);
        write_str(buf);
        ESP_LOGD(TAG, buf);
    }
        
  private: 
    String MarlinOutput;
    String StateText;
    String S_time;
    unsigned long millisProgress=0;
    unsigned long total=0;
    float percentDone=0;
    uint8_t state=CMU_STATE_UNKNOWN;
    

    component_MarlinUART(UARTComponent *parent) : PollingComponent(2000), UARTDevice(parent) 
    {
       
        this->sensor_bedtemp = new Sensor();
        this->sensor_bedsetpoint = new Sensor();
        
        this->sensor_exttemp = new Sensor();
        this->sensor_extsetpoint = new Sensor();
        
        this->sensor_progress = new Sensor();
    
        this->textsensor_printerState = new TextSensor();
        this->textsensor_elapsedTime = new TextSensor();
        this->textsensor_remainingTime = new TextSensor();

    }
    
    void set_state(uint8_t newstate)  {
        
        if (state==newstate)
            return;
        
        switch (newstate)  {
            case CMU_STATE_UNKNOWN:
                textsensor_printerState->publish_state("Unknown");
                textsensor_elapsedTime->publish_state("Unknown");
                textsensor_remainingTime->publish_state("Unknown");
                sensor_exttemp->publish_state(NAN);
                sensor_extsetpoint->publish_state(NAN);
                sensor_bedtemp->publish_state(NAN);
                sensor_bedsetpoint->publish_state(NAN);
                sensor_progress->publish_state(NAN);
                break;
        
            case CMU_STATE_IDLE:
                textsensor_printerState->publish_state("Idle");
                textsensor_elapsedTime->publish_state("Unknown");
                textsensor_remainingTime->publish_state("Unknown");
                sensor_progress->publish_state(NAN);
                break;
        
            case CMU_STATE_PREHEAT:
                textsensor_printerState->publish_state("Preheat");
                textsensor_elapsedTime->publish_state("Unknown");
                textsensor_remainingTime->publish_state("Unknown");
                sensor_progress->publish_state(NAN);
                break;
        
            case CMU_STATE_PRINTING:
                textsensor_printerState->publish_state("Printing");
                break;
        
            case CMU_STATE_RUNOUT:
                textsensor_printerState->publish_state("Runout");
                break;
        
            case CMU_STATE_PAUSED:
                textsensor_printerState->publish_state("Paused");
                break;
        
            case CMU_STATE_ABORT:
                textsensor_printerState->publish_state("Aborted");
                textsensor_elapsedTime->publish_state("Unknown");
                textsensor_remainingTime->publish_state("Unknown");
                sensor_progress->publish_state(NAN);
                break;
        
            case CMU_STATE_FINISHED:
                textsensor_printerState->publish_state("Finished");
                break;
        
            case CMU_STATE_HALTED:
                textsensor_printerState->publish_state("Halted");
                textsensor_elapsedTime->publish_state("Unknown");
                textsensor_remainingTime->publish_state("Unknown");
                sensor_exttemp->publish_state(NAN);
                sensor_extsetpoint->publish_state(NAN);
                sensor_bedtemp->publish_state(NAN);
                sensor_bedsetpoint->publish_state(NAN);
                sensor_progress->publish_state(NAN);
                break;
        
            case CMU_STATE_COOLING:
                textsensor_printerState->publish_state("Cooling");
                textsensor_elapsedTime->publish_state("Unknown");
                textsensor_remainingTime->publish_state("Unknown");
                sensor_progress->publish_state(NAN);                
                break;
                
        }
        
        state = newstate;

    }

    int process_tempfeedback(float* et, float* es, float* bt, float* bs) {
        float dc; //don't care

        //Preprocess remove all spaces and leading ok
        MarlinOutput.replace(" ", "");
        MarlinOutput.replace("ok", "");

        //T:157.35/0.00B:56.56/0.00@:0B@:0
        //T:201/202B:117/120
        //T:201/202B:117/120C:49.3/50
        //T:20.2/0.0B:19.1/0.0T0:20.2/0.0@:0B@:0P:19.8A:26.4         
        if(sscanf(MarlinOutput.c_str() ,"T:%f/%fB:%f/%f", et, es, bt, bs) == 4 )
            return 1;

        //T:219.64/220.00(313.7500)B:60.05/60.00(3426.9375)@:86B@:46
        if(sscanf(MarlinOutput.c_str() ,"T:%f/%f(%f)B:%f/%f(%f)", et, es, &dc, bt, bs, &dc) == 6 )
            return 2;

        //T:201/202T0:110/110T1:23/0B:117/120C:49.3/50
        if(sscanf(MarlinOutput.c_str() ,"T:%f/%fT0:%f/%fT1:%f/%fB:%f/%f", et, es, &dc, &dc, &dc, &dc, bt, bs) == 8 )
            return 3;  
            
        //T0:110/110T1:23/0B:117/120
        if(sscanf(MarlinOutput.c_str() ,"T0:%f/%fT1:%f/%fB:%f/%f", et, es, &dc, &dc, bt, bs) == 6 )
            return 4;                       

        return 0;
    }
  
    void process_line() {
        
        ESP_LOGD(TAG, MarlinOutput.c_str() );
        
        // Auto reported temperature format without SHOW_TEMP_ADC_VALUES defined in Marlin configuration
        // T:157.35 /0.00 B:56.56 /0.00 @:0 B@:0
        
        // Auto reported temperature format with SHOW_TEMP_ADC_VALUES defined in Marlin configuration
        // T:219.64 /220.00 (313.7500) B:60.05 /60.00 (3426.9375) @:86 B@:46

        //https://reprap.org/wiki/G-code#M105:_Get_Extruder_Temperature
        // ok T:201 /202 B:117 /120
        // ok T:201 /202 B:117 /120 C:49.3 /50
        // ok T:201 /202 T0:110 /110 T1:23 /0 B:117 /120 C:49.3 /50
        // ok T0:110 /110 T1:23 /0 B:117 /120
        // ok T:20.2 /0.0 B:19.1 /0.0 T0:20.2 /0.0 @:0 B@:0 P:19.8 A:26.4

        if(MarlinOutput.startsWith(String(" T:"))   || 
           MarlinOutput.startsWith(String("T:"))    ||
           MarlinOutput.startsWith(String("ok T:")) ||
           MarlinOutput.startsWith(String(" ok T:"))   ) {

            float extruderTemp, extruderSet, bedTemp, bedSet;
            if (process_tempfeedback(&extruderTemp, &extruderSet, &bedTemp, &bedSet) != 0)  {
                sensor_exttemp->publish_state(extruderTemp);
                sensor_extsetpoint->publish_state(extruderSet);
                sensor_bedtemp->publish_state(bedTemp);
                sensor_bedsetpoint->publish_state(bedSet);
                
                if(bedSet==0.0 && extruderSet==0.0)  {
                    if(extruderTemp < 32.0 && bedTemp < 32.0)         //TODO define constants for these
                        set_state(CMU_STATE_IDLE);
                    else if(extruderTemp < 150.0 && bedTemp < 55.0)
                        set_state(CMU_STATE_COOLING);
                }
                if(bedSet!=0.0 || extruderSet!=0.0)  {
                    if(state == CMU_STATE_COOLING || state == CMU_STATE_IDLE)
                        set_state(CMU_STATE_PREHEAT);
                }
                
            }
            MarlinOutput="";
            return;
        }
        
        //SD printing byte 2467546/3281364
       if(MarlinOutput.startsWith(String("SD printing byte "))) {
            long current, total;
            
            current = MarlinOutput.substring(17).toInt();
            total = MarlinOutput.substring(MarlinOutput.indexOf('/')+1).toInt();
            
            //ESP_LOGD(TAG,String(current).c_str());
            //ESP_LOGD(TAG,String(total).c_str());
            
            if (total==0)  {
                sensor_progress->publish_state(NAN);
                percentDone=0.0;
            }
            else  {
                percentDone = (float) current / (float) total;
                sensor_progress->publish_state( percentDone * 100.0);
            }
            
            MarlinOutput="";
            return;
        } 
        
        //echo:Print time: 
        //echo:Print time: 3h 6m 49s
       if(MarlinOutput.startsWith(String("echo:Print time: "))) {
            int d=0, h=0, m=0, s=0;
            unsigned long current=0, remaining=0;
            
            S_time = MarlinOutput.substring(16);
            
            //ESP_LOGD(TAG,S_time.c_str());
            
            if (sscanf(S_time.c_str() ,"%dd %dh %dm %ds", &d, &h, &m, &s)!=4)  {
                d=0;
                if (sscanf(S_time.c_str() ,"%dh %dm %ds", &h, &m, &s)!=3)  {
                    d=0; h=0;
                    if (sscanf(S_time.c_str() ,"%dm %ds", &m, &s)!=2)  {
                        d=0; h=0; m=0;
                        if (sscanf(S_time.c_str() ,"%ds", &s)!=1)  {
                            MarlinOutput="";
                            return;
                        }
                    }
                }
            }
            
            current = d*24*60*60 + h*60*60 + m*60 + s;
            //ESP_LOGD(TAG,String(current).c_str());
            
            textsensor_elapsedTime->publish_state(S_time.c_str());
            
            //Start G-Code needs to include M77 & M75 to stop and restart print timer
            //once bed & extruder are heated.  Otherwise preheat time is included and 
            //messes up the estimate considerably.
            
            //TODO:  see if runout and pauses stops elapsed print timer
            
            if(percentDone != 0.0 && percentDone != 100.0) {
                if(total < current)  //if slicer estimate is 0 or value has been exceeded default to estimated time remaining
                    remaining = ((float) current / percentDone) - current;  //estimate total from elapsed and progress
                else
                    remaining = total - current;  //use slicer estimate
                
                if (remaining > (60*60) ) {
                    S_time = String( (float) remaining / 3600.0, 2);
                    S_time += " Hours";
                }
                else  {
                    S_time = String( (float) remaining / 60.0, 2);
                    S_time += " Minutes";
                }
                
                textsensor_remainingTime->publish_state(S_time.c_str());
            }           
            MarlinOutput="";
            return;
        }  

            
        //State changes //////////////////////////////
        
        //action:prompt_begin FilamentRunout T0
        if(MarlinOutput.indexOf(String("FilamentRunout")) != -1) {
            set_state(CMU_STATE_RUNOUT);
            MarlinOutput="";
            return;
        }
        
        if(MarlinOutput.startsWith(String("echo:busy: paused"))) {
            if(state != CMU_STATE_RUNOUT) {
                set_state(CMU_STATE_PAUSED);
            }
            MarlinOutput="";
            return;
        }
                
        if(MarlinOutput.startsWith(String("Done printing"))) {
            set_state(CMU_STATE_FINISHED);
            textsensor_remainingTime->publish_state("0.00 Minutes");
            percentDone=100.0;
            total=0;
            sensor_progress->publish_state(100.0);
            MarlinOutput="";
            return;
        }
        
        //TODO when extruder and bed are fully preheated this doesn't seem to get sent
        if(MarlinOutput.startsWith(String("//action:resume"))) {
            set_state(CMU_STATE_PRINTING);
            MarlinOutput="";
            return;
        }
        
        //Cutoff text gets sent when extruder and nozzle are fully preheated when
        //SD print is started.
        if(MarlinOutput.startsWith(String("ction:resume"))) {
            set_state(CMU_STATE_PRINTING);
            MarlinOutput="";
            return;
        }
        
        //Estimated print time 00:44:14;
        //Estimated print time 27:57:55;
        if(MarlinOutput.startsWith(String("Estimated print time "))) {
            int h=0, m=0, s=0;
            S_time = MarlinOutput.substring(20);
            
            //ESP_LOGD(TAG,S_time.c_str());
            
            if (sscanf(S_time.c_str() ,"%d:%d:%d", &h, &m, &s)!=3)  {
                total = 0;
                MarlinOutput="";
                return;
            }
            
            total = h*60*60 + m*60 + s;
        }
        
        if(MarlinOutput.startsWith(String("Error:Printer halted"))) {
            set_state(CMU_STATE_HALTED);
            MarlinOutput="";
            return;
        }

        if(MarlinOutput.startsWith(String("//action:notification Print Aborted"))) {
            set_state(CMU_STATE_ABORT);
            MarlinOutput="";
            return;
        }        
        
        //reset string for next line
        MarlinOutput="";
    }
};
    

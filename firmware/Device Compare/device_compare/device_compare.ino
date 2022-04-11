// Test sketch for comparing devices


#define DEVICE_TYPE "Ribbit Embedded 0.1"
#define DEVICE_NUMBER "1"
#define REST_TIME (15) //Rest time between reading from the sensors in seconds
#define REST_TIME_MS (REST_TIME * 1000)
#define POST_TIME (300) //time between posts to the internet in seconds
#define NUM_LOOPS (POST_TIME / REST_TIME) //how many loops do we need to run before posting?

// ---------------------------------------------------------------------------------------------------------------
// ------------------------------------------- Wifi and InfluxDB -------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------
#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;

#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

// WiFi AP SSID
#define WIFI_SSID "*******"
// WiFi password
#define WIFI_PASSWORD "********"
// InfluxDB v2 server url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
#define INFLUXDB_URL "https://westeurope-1.azure.cloud2.influxdata.com"
// InfluxDB v2 server or cloud API token (Use: InfluxDB UI -> Data -> API Tokens -> Generate API Token)
#define INFLUXDB_TOKEN "58t3r9VVAbyukxoprSdpBbsLCiawUc0SEB54IXdQK-MhhNaBl65uY3kWxkvcBdhb-m_Wwd5kVzNr8XkICp3yiQ==" //ESP Device Compare Token
// InfluxDB v2 organization id (Use: InfluxDB UI -> User -> About -> Common Ids )
#define INFLUXDB_ORG "derek.gutheil@gmail.com"
// InfluxDB v2 bucket name (Use: InfluxDB UI ->  Data -> Buckets)
#define INFLUXDB_BUCKET "Device Compare Bucket"

// Set timezone string according to https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
// Examples:
//  Pacific Time: "PST8PDT"
//  Eastern: "EST5EDT"
//  Japanesse: "JST-9"
//  Central Europe: "CET-1CEST,M3.5.0,M10.5.0/3"
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Data point
Point sensor("wifi_status");



// ---------------------------------------------------------------------------------------------------------------
// ------------------------------------ Pressure and Temperature -------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------
#include <Dps310.h>

#define PASCAL_TO_MILLIBAR (0.01)

// Dps310 Opject
Dps310 Dps310PressureSensor = Dps310();

float averageTemperatureArray[NUM_LOOPS];
float averagePressureArray[NUM_LOOPS];
float averageTemperature;
float averagePressure;

uint8_t loopCounter;


void setup() {

  //turn off bluetooth
  btStop();
  
  loopCounter = 0u;
  averageTemperature = 0.0;
  averagePressure = 0.0;

  for(int i = 0; i < NUM_LOOPS; i++)
  {
    averageTemperatureArray[i] = 0.0;
    averagePressureArray[i] = 0.0;
  }
  
  Serial.begin(115200);

  // ---------------------------------------------------------------------------------------------------------------
  // ------------------------------------ Pressure and Temperature -------------------------------------------------
  // ---------------------------------------------------------------------------------------------------------------
  Serial.println("DPS310");
  Dps310PressureSensor.begin(Wire);

  //temperature measure rate (value from 0 to 7)
  //2^temp_mr temperature measurement results per second
  int16_t temp_mr = 0;
  //temperature oversampling rate (value from 0 to 7)
  //2^temp_osr internal temperature measurements per result
  //A higher value increases precision
  int16_t temp_osr = 0;
  //pressure measure rate (value from 0 to 7)
  //2^prs_mr pressure measurement results per second
  int16_t prs_mr = 0;
  //pressure oversampling rate (value from 0 to 7)
  //2^prs_osr internal pressure measurements per result
  //A higher value increases precision
  int16_t prs_osr = 0;
  int16_t ret = Dps310PressureSensor.startMeasureBothCont(temp_mr, temp_osr, prs_mr, prs_osr);

  //This setup should get us 1 measurement per second with no oversample
  if (ret != 0)
  {
    Serial.print("DPS310 Init FAILED! ret = ");
    Serial.println(ret);
  }
  else
  {
    Serial.println("DPS310 Init complete!");
  }


  // ---------------------------------------------------------------------------------------------------------------
  // ------------------------------------------- Wifi and InfluxDB -------------------------------------------------
  // ---------------------------------------------------------------------------------------------------------------

  // Setup wifi
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  // Add tags
  sensor.addTag("device_type", DEVICE_TYPE);
  sensor.addTag("device_number", DEVICE_NUMBER);

  // Accurate time is necessary for certificate validation and writing in batches
  // For the fastest time sync find NTP servers in your area: https://www.pool.ntp.org/zone/
  // Syncing progress and the time will be printed to Serial.
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}




void loop() 
{
  
  // ---------------------------------------------------------------------------------------------------------------
  // ------------------------------------ Pressure and Temperature -------------------------------------------------
  // ---------------------------------------------------------------------------------------------------------------
  uint8_t pressureCount = 32;
  float pressure[pressureCount];
  uint8_t temperatureCount = 32;
  float temperature[temperatureCount];

  float tempAverageTemperature = 0.0;
  float tempAveragePressure = 0.0;

  //This function writes the results of continuous measurements to the arrays given as parameters
  //The parameters temperatureCount and pressureCount should hold the sizes of the arrays temperature and pressure when the function is called
  //After the end of the function, temperatureCount and pressureCount hold the numbers of values written to the arrays
  //Note: The Dps310 cannot save more than 32 results. When its result buffer is full, it won't save any new measurement results
  int16_t ret = Dps310PressureSensor.getContResults(temperature, temperatureCount, pressure, pressureCount);

  if (ret != 0)
  {
    Serial.println();
    Serial.println();
    Serial.print("FAIL! ret = ");
    Serial.println(ret);
  }
  else
  {
    Serial.println();
    Serial.println();
    Serial.print(temperatureCount);
    Serial.println(" temperature values found: ");
    for (int16_t i = 0; i < temperatureCount; i++)
    {
      Serial.print(temperature[i]);
      Serial.println(" degrees of Celsius");
      tempAverageTemperature += temperature[i];
    }

    Serial.println();
    Serial.print(pressureCount);
    Serial.println(" pressure values found: ");
    for (int16_t i = 0; i < pressureCount; i++)
    {
      Serial.print(pressure[i] * PASCAL_TO_MILLIBAR);
      Serial.println(" mBar");
      tempAveragePressure += pressure[i] * PASCAL_TO_MILLIBAR;
    }

    tempAverageTemperature /= temperatureCount;
    tempAveragePressure /= pressureCount;

    Serial.print("Average Temperature: ");
    Serial.print(tempAverageTemperature);
    Serial.println(" *C");
    Serial.print("Average Pressure: ");
    Serial.print(tempAveragePressure);
    Serial.println(" mBar");
    Serial.print("Loop Counter: ");
    Serial.println(loopCounter);

    // add these temperary values to the array to be averaged later
    averageTemperatureArray[loopCounter] = tempAverageTemperature;
    averagePressureArray[loopCounter] = tempAveragePressure;
  }



  //we hit a transmit loop!
  if (loopCounter >= (NUM_LOOPS - 1))
  {
    
    //Calculate final averages
    averageTemperature = 0.0;
    averagePressure = 0.0;
    for (int i = 0; i < loopCounter; i++)
    {
      averageTemperature += averageTemperatureArray[i];
      averagePressure += averagePressureArray[i];
      Serial.print("averageTemperatureArray[i]: ");
      Serial.println(averageTemperatureArray[i]);
      Serial.print("averageTemperature: ");
      Serial.println(averageTemperature);
      
      Serial.print("averagePressureArray[i]: ");
      Serial.println(averagePressureArray[i]);
      Serial.print("averagePressure: ");
      Serial.println(averagePressure);
    }
    averageTemperature /= loopCounter;
    averagePressure /= loopCounter;

    Serial.print("loopCounter: ");
    Serial.println(loopCounter);

    Serial.print("Final Average Temperature: ");
    Serial.print(averageTemperature);
    Serial.println(" *C");
    Serial.print("Final Average Pressure: ");
    Serial.print(averagePressure);
    Serial.println(" mBar");
    

    //transmit stuff
    
    // ---------------------------------------------------------------------------------------------------------------
    // ------------------------------------------- InfluxDB ----------------------------------------------------------
    // ---------------------------------------------------------------------------------------------------------------
    
    // Clear fields for reusing the point. Tags will remain untouched
    sensor.clearFields();
  
    // Store measured value into point
  
    //add temperature and pressure info
    sensor.addField("Temperature", averageTemperature);
    sensor.addField("Pressure", averagePressure);
  
    // Print what are we exactly writing
    Serial.print("Writing: ");
    Serial.println(sensor.toLineProtocol());
  
    // Check WiFi connection and reconnect if needed
    if (wifiMulti.run() != WL_CONNECTED) {
      Serial.println("Wifi connection lost");
    }
  
    // Write point
    if (!client.writePoint(sensor)) {
      Serial.print("InfluxDB write failed: ");
      Serial.println(client.getLastErrorMessage());
    }

    //reset the loop counter
    loopCounter = 0u;

    //reset averages and such
    averageTemperature = 0.0;
    averagePressure = 0.0;
  
    for(int i = 0; i < NUM_LOOPS; i++)
    {
      averageTemperatureArray[i] = 0.0;
      averagePressureArray[i] = 0.0;
    }
  }
  else
  {
     //increment the loop counter
     loopCounter += 1u;
  }



 

  // ---------------------------------------------------------------------------------------------------------------
  // ------------------------------------------- Loop Delay --------------------------------------------------------
  // ---------------------------------------------------------------------------------------------------------------
  Serial.print("Wait ");
  Serial.print(REST_TIME);
  Serial.println(" seconds");
  delay(REST_TIME_MS);
}

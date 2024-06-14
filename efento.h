#include "esphome.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <string>
#include <map>

using namespace esphome;

struct EfentoSensorData
{
    std::string type;
    float min;
    float max;
    float resolution;
    std::string unit;
    bool binary;
    int metadata_factor;
    std::string summary;
};

std::map<int, EfentoSensorData> sensorDataMap = {
   {1,  {"TEMPERATURE",            -273.2f,    4000.0f,    0.1f,       "°C",           false,  1,  "Temperature"}},
   {2,  {"HUMIDITY",               0,          100,        1.0f,       "%",            false,  1,  "Humidity"}},
   {3,  {"ATMOSPHERIC_PRESSURE",   1.0f,       2000.0f,    0.1f,       "hPa",          false,  1,  "Atmospheric pressure"}},
   {4,  {"DIFFERENTIAL_PRESSURE",  -10000,     10000,      1.0f,       "Pa",           false,  1,  "Differential pressure"}},
   {5,  {"OK_ALARM",               0,          1,          1,          "-",            true,   1,  "Binary format for OK/Alarm"}},
   {6,  {"IAQ",                    0,          500,        1.0f,       "index",        false,  1,  "Indoor air quality"}},
   {7,  {"FLOODING",               0,          1,          1,          "-",            true,   1,  "Flooding"}},
   {8,  {"PULSE_CNT",              0,          10000,      1,          "pulses",       false,  1,  "Pulse count"}},
   {9,  {"ELECTRICITY_METER",      0,          10000,      1.0f,       "kWh",          false,  1,  "Electricity consumption"}},
   {10, {"WATER_METER",            0,          10000,      1.0f,       "liters",       false,  1,  "Water consumption"}},
   {11, {"SOIL_MOISTURE",          0,          100,        1.0f,       "%",            false,  1,  "Soil moisture"}},
   {12, {"CO_GAS",                 0,          1000000,    1.0f,       "ppm",          false,  1,  "Carbon monoxide"}},
   {13, {"NO2_GAS",                0,          1000000,    1.0f,       "ppm",          false,  1,  "Nitrogen dioxide"}},
   {14, {"H2S_GAS",                0,          80000.00f,  0.01,       "ppm",          false,  1,  "Hydrogen sulfide"}},
   {15, {"AMBIENT_LIGHT",          0,          100000.0f,  0.1f,       "lx",           false,  1,  "Illuminance"}},
   {16, {"PM_1_0",                 0,          1000,       1.0f,       "µg/m^3",       false,  1,  "Mass concentration of particles less than 1µm"}},
   {17, {"PM_2_5",                 0,          1000,       1.0f,       "µg/m^3",       false,  1,  "Mass concentration of particles less than 2.5µm"}},
   {18, {"PM_10_0",                0,          1000,       1.0f,       "µg/m^3",       false,  1,  "Mass concentration of particles less than 10µm"}},
   {19, {"NOISE_LEVEL",            0,          200.0f,     0.1f,       "dB",           false,  1,  "Noise level"}},
   {20, {"NH3_GAS",                0,          1000000,    1.0f,       "ppm",          false,  1,  "Ammonia"}},
   {21, {"CH4_GAS",                0,          1000000,    1.0f,       "ppm",          false,  1,  "Methane"}},
   {22, {"HIGH_PRESSURE",          0,          200000,     1.0f,       "kPa",          false,  1,  "High pressure"}},
   {23, {"DISTANCE_MM",            0,          100000,     1.0f,       "mm",           false,  1,  "Distance"}},
   {24, {"WATER_METER_ACC_MINOR",  0,          99,         1.0f,       "liter",        false,  6,  "Accumulative water meter (pulse counter)"}},
   {25, {"WATER_METER_ACC_MAJOR",  0,          999999,     1.0f,       "hectoliter",   false,  4,  "Accumulative water meter (pulse counter)"}},
   {26, {"CO2_GAS",                0,          1000000,    1.0f,       "ppm",          false,  3,  "Carbon dioxide"}},
   {27, {"HUMIDITY_ACCURATE",      0,          100.0f,     0.1f,       "%",            false,  1,  "Humidity"}},
   {28, {"STATIC_IAQ",             0,          10000,      1.0f,       "-",            false,  3,  "Static indoor air quality"}},
   {29, {"CO2_EQUIVALENT",         0,          1000000,    1.0f,       "ppm",          false,  3,  "Carbon dioxide equivalent estimate"}},
   {30, {"BREATH_VOC",             0,          100000,     1.0f,       "ppm",          false,  3,  "Breath-VOC estimate"}},
   // {31, {"CELLULAR_GATEWAY",       0,          4194303,    "-",        "-",            false,  1,  "Cellular gateway"}},
   {32, {"PERCENTAGE",             0,          100.00f,    0.01,       "%",            false,  1,  "Percentage"}},
   {33, {"VOLTAGE",                0,          100000.0f,  0.1f,       "mV",           false,  1,  "Voltage"}},
   {34, {"CURRENT",                0,          10000.00f,  0.01,       "mA",           false,  1,  "Current"}},
   {35, {"PULSE_CNT_ACC_MINOR",    0,          999,        1.0f,       "Pulses",       false,  6,  "Accumulative pulse counter"}},
   {36, {"PULSE_CNT_ACC_MAJOR",    0,          999999,     1.0f,       "Kilo Pulses",  false,  4,  "Accumulative pulse counter"}},
   {37, {"ELEC_METER_ACC_MINOR",   0,          999,        1.0f,       "Wh",           false,  6,  "Accumulative electricity meter (pulse counter)"}},
   {38, {"ELEC_METER_ACC_MAJOR",   0,          999999,     1.0f,       "kWh",          false,  4,  "Accumulative electricity meter (pulse counter)"}}};

std::vector<uint8_t> processAdvertisement(const esp32_ble_tracker::ESPBTDevice &device)
{
    std::vector<uint8_t> combinedFrames;

    for (const auto &data : device.get_manufacturer_datas())
    {
        combinedFrames.insert(combinedFrames.end(), data.data.begin(), data.data.end());
    }

    return combinedFrames;
}

void logProcessedAdvertisement(const std::vector<uint8_t> &vec)
{
    std::stringstream ss;

    ss << "Processed advertisement: [";
    for (const auto &byte : vec)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << " ";
    }
    ss.seekp(-1, ss.cur); // Remove the last space
    ss << "]";

    ESP_LOGD("Efento", "%s", ss.str().c_str());
}

std::string getFirmwareVersion(const std::vector<uint8_t> &data)
{
    if (data.size() < 9)
        return "Invalid firmware version data";

    uint16_t value = (static_cast<uint16_t>(data[7]) << 8) | data[8];
    return std::to_string((value >> 11) & 0x1F) + "." +
           std::to_string((value >> 5) & 0x3F) + "." +
           std::to_string(value & 0x1F);
}

/**
 * Gets the battery level from a provided data vector.
 *
 * This function checks the least significant bit of the 10th element (index 9)
 * in the data vector. This bit represents the battery level status.
 * If the bit is set (1), the function returns "OK" indicating the battery level is good.
 * If the bit is not set (0), the function returns "Low" indicating a low battery level.
 *
 * @param data The vector containing the battery level status at index 9.
 * @return A string representing the battery level: "OK" for a good level or "Low" for a low level.
 */
std::string getBatteryLevel(std::vector<uint8_t> &data)
{
    if (data.size() < 10)
        return "Invalid battery level data";

    return data[9] & 0x01 ? "OK" : "Low";
}

std::string getExternalPowerSupplyStatus(std::vector<uint8_t> &data)
{
    if (data.size() < 10)
        return "Invalid power supply status data";

    switch ((data[9] >> 1) & 0x03)
    {
    case 0:
        return "Battery only";
    case 1:
        return "External power supply connected";
    case 2:
        return "External power supply disconnected";
    case 3:
        return "Power supply error";
    }

    return "Invalid power supply status";
}

std::string getEncryptionStatus(std::vector<uint8_t> &data)
{
    if (data.size() < 10)
        return "Invalid encryption status data";

    return (data[9] >> 3) & 0x01 ? "Yes" : "No";
}

std::string getTimeSynchronizationStatus(std::vector<uint8_t> &data)
{
    if (data.size() < 10)
        return "Invalid time synchronization status data";

    return (data[9] >> 4) & 0x01 ? "Yes" : "No";
}

std::string getRuntimeErrorOrModemLoggingStatus(std::vector<uint8_t> &data)
{
    if (data.size() < 10)
        return "Invalid runtime error or modem logging status data";

    return (data[9] >> 5) & 0x01 ? "Yes" : "No";
}

std::string getCellularStatus(std::vector<uint8_t> &data)
{
    if (data.size() < 10)
        return "Invalid cellular status data";

    switch ((data[9] >> 6) & 0x03)
    {
    case 0:
        return "BLE only";
    case 1:
        return "Cellular connection OK";
    case 2:
        return "No server connection";
    case 3:
        return "Network issue / not registered";
    }

    return "Invalid cellular status";
}

std::string getMeasurementTimestamp(const std::vector<uint8_t> &data)
{
    if (data.size() < 14)
        return "Invalid measurement timestamp data";

    uint32_t measurementTimestamp = 0;

    for (size_t i = 10; i <= 13; ++i)
    {
        measurementTimestamp = (measurementTimestamp << 8) | static_cast<uint32_t>(data[i]);
    }

    time_t raw_time = static_cast<time_t>(measurementTimestamp);
    std::tm *time_info = std::localtime(&raw_time);

    std::ostringstream oss;
    oss << std::put_time(time_info, "%d-%m-%y %H:%M:%S");
    return oss.str();
}

uint16_t getMeasurementPeriodBase(const std::vector<uint8_t> &data)
{
    if (data.size() < 14)
        return 0;

    return (static_cast<uint16_t>(data[14]) << 8) | data[15];
}

uint16_t getMeasurementPeriodFactor(const std::vector<uint8_t> &data)
{
    if (data.size() < 14)
        return 0;

    return (static_cast<uint16_t>(data[16]) << 8) | data[17];
}

uint32_t getMeasurementPeriod(const std::vector<uint8_t> &data)
{
    uint16_t measurementPeriodBase = getMeasurementPeriodBase(data);
    uint16_t measurementPeriodFactor = getMeasurementPeriodFactor(data);

    return static_cast<uint32_t>(measurementPeriodBase) * measurementPeriodBase;
}

uint16_t getCalibrationDate(const std::vector<uint8_t> &data)
{
    if (data.size() < 19)
        return 0;

    return (static_cast<uint16_t>(data[18]) << 8) | data[19];
}

uint8_t getChannelTypeByte(const std::vector<uint8_t> &data, uint8_t channel)
{
    size_t index = 23 + 4 * (static_cast<size_t>(channel) - 1);
    return data[index];
}

uint32_t getChannelMeasurementBytes(const std::vector<uint8_t> &data, uint8_t channel)
{
    size_t index = 24 + 4 * (static_cast<size_t>(channel) - 1);
    return (static_cast<uint32_t>(data[index]) << 16) | (static_cast<uint32_t>(data[index + 1]) << 8) | data[index + 2];
}

EfentoSensorData getChannelDetails(const std::vector<uint8_t> &data, uint8_t channel)
{
    uint8_t typeByte = getChannelTypeByte(data, channel);
    return sensorDataMap[static_cast<int>(typeByte)];
}

int decodeZigzag(int n)
{
    return (n >> 1) ^ -(n & 1);
}

float getMeasurementValue(const std::vector<uint8_t> &data, uint8_t channel)
{
    if ((channel < 1) || (channel > 6))
    {
        return 0;
    }

    EfentoSensorData channelDetails = getChannelDetails(data, channel);
    uint32_t measurementBytes = getChannelMeasurementBytes(data, channel);

    return (decodeZigzag(measurementBytes) / channelDetails.metadata_factor) * channelDetails.resolution;
}

int getMetadataValue(const std::vector<uint8_t> &data, uint8_t channel)
{
    if ((channel < 1) || (channel > 6))
    {
        return 0;
    }

    EfentoSensorData channelDetails = getChannelDetails(data, channel);
    uint32_t measurementBytes = getChannelMeasurementBytes(data, channel);

    return abs(decodeZigzag(measurementBytes)) % channelDetails.metadata_factor;
}

std::string getType(const std::vector<uint8_t> &data, uint8_t channel)
{
    if (channel < 1 || channel > 6)
    {
        return "Invalid sensor type";
    }

    return getChannelDetails(data, channel).type;
}

float getMin(const std::vector<uint8_t> &data, uint8_t channel)
{
    if (channel < 1 || channel > 6)
    {
        return 0;
    }

    return getChannelDetails(data, channel).min;
}

float getMax(const std::vector<uint8_t> &data, uint8_t channel)
{
    if (channel < 1 || channel > 6)
    {
        return 0;
    }

    return getChannelDetails(data, channel).max;
}

float getResolution(const std::vector<uint8_t> &data, uint8_t channel)
{
    if (channel < 1 || channel > 6)
    {
        return 0;
    }

    return getChannelDetails(data, channel).resolution;
}

std::string getUnit(const std::vector<uint8_t> &data, uint8_t channel)
{
    if (channel < 1 || channel > 6)
    {
        return "Invalid sensor type";
    }

    return getChannelDetails(data, channel).unit;
}

std::string getIsBinary(const std::vector<uint8_t> &data, uint8_t channel)
{
    if (channel < 1 || channel > 6)
    {
        return "No";
    }

    return getChannelDetails(data, channel).binary ? "Yes" : "No";
}

int getMetadataFactor(const std::vector<uint8_t> &data, uint8_t channel)
{
    if (channel < 1 || channel > 6)
    {
        return 0;
    }

    return getChannelDetails(data, channel).metadata_factor;
}

std::string getSummary(const std::vector<uint8_t> &data, uint8_t channel)
{
    if (channel < 1 || channel > 6)
    {
        return "Invalid sensor type";
    }

    return getChannelDetails(data, channel).summary;
}

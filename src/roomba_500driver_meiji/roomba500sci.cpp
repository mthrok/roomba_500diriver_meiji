//---------------------------< /-/ AMSL /-/ >------------------------------
/**
 * file         :       roomba500sci.cpp
 *
 *
 * Environment  :       g++
 * Latest Update:       2011/05/24
 *
 * Designer(s)  :       t.saitoh (AMSL)
 * Author(s)    :       t.saitoh (AMSL)
 *
 * CopyRight    :       2011, Autonomous Mobile Systems Laboratory, Meiji Univ.
 *
 * Revision     :       2011/05/24
 *
 */
//-----------------------------------------------------------------------------

#include "roomba_500driver_meiji/roomba500sci.hpp"

#include <stdio.h>
#include <cmath>
#include <string>
#include <unistd.h>
#include <stdexcept>

using namespace roombaC2;
using namespace std;

typedef roomba_500driver_meiji::RoombaSensors RoombaSensors;
typedef roomba_500driver_meiji::RoombaCtrl RoombaCtrl;

void sleep_for_sec (float sec){
  long usec=(long)(sec*1000000);
  usleep(usec);
}

// TODO Add other member variables to initalization list
Roomba::Roomba()
  : stopStateManager_(true)
  , stateManager_()
{}

Roomba::~Roomba() {
  // Stop state manager
  stopStateManager_ = true;
  stateManager_.join();
  // Delete serial communication
  delete comm_;
}

void Roomba::init(int baud, const char* dev) {
  comm_ = new Serial(baud, dev, 80, 0);
  sleep(1);
  startStateUpdater();
};

void Roomba::sendOpCode(OPCODE oc, const uint8* dataBytes,
			uint nDataBytes) const{
  // Concatenate operation code and databytes
  uint nMsg = nDataBytes + 1;
  uint8 *message = new uint8[nMsg];
  message[0] = (uint8)oc;
  if (dataBytes)
    memcpy(message+1, dataBytes, nDataBytes);
  // Send message
  if (nMsg != comm_ -> write(message, nMsg)) {
    //std::string err_msg(__func__); err_msg += ":Failed to send command.";
    //throw std::runtime_error(err_msg.c_str());
  }
  sleep_for_sec(COMMAND_WAIT);
}


void Roomba::updateState() {
  while(!stopStateManager_) {
    // Retreive sensor values
    updateSensorState();
    printf("Encoder count L/R: %8d / %8d\n", sensor_.encoder_counts.left, sensor_.encoder_counts.right);
    float distance = sensor_.travel.distance / 1000.0;
    uint16 angle = sensor_.travel.angle;
    printf("Distance Traveled: %8.3f [m]/ %8d [deg]\n", distance, angle);

  /*
  // Update internal encoder counts
  if(std::abs((int)sensor.encoder_counts.right - (int)enc_count_r_) >= 60000) {
    if(sensor.encoder_counts.right > enc_count_r_)
      d_enc_count_r_ = -65535 - enc_count_r_ + sensor.encoder_counts.right;
    else
      d_enc_count_r_ = 65535 - enc_count_r_ + sensor.encoder_counts.right;
  } else {
    d_enc_count_r_ = sensor.encoder_counts.right - enc_count_r_;
  }

  if(std::abs((int)sensor.encoder_counts.left - (int)enc_count_l_) >= 60000) {
    if(sensor.encoder_counts.left > enc_count_l_)
      d_enc_count_l_ = -65535-enc_count_l_+sensor.encoder_counts.left;
    else
      d_enc_count_l_ = 65535-enc_count_l_+sensor.encoder_counts.left;
  } else {
    d_enc_count_l_ = sensor.encoder_counts.left - enc_count_l_;
  }

  enc_count_r_ = sensor.encoder_counts.right;
  enc_count_l_ = sensor.encoder_counts.left;
  */



    /*
    // Correct encoder values
    int enc_r = dEncoderRight();
    int enc_l = dEncoderLeft();
    if(abs(enc_r) == 200)
      enc_r = pre_enc_r;
    if(abs(enc_l) == 200)
      enc_l = pre_enc_l;

    float distance = ((float)enc_r + enc_l) / 2270.0 * 0.5;
    float angle = ((float)enc_r - enc_l) / 2270.0 / 0.235;
    sens.travel.distance = distance;
    sens.travel.angle = angle;
    */
    //
    sleep_for_sec(1.0 / 10);
  }
}

void Roomba::startStateUpdater() {
  stopStateManager_ = false;
  stateManager_ = boost::thread(&Roomba::updateState, this);
}

void Roomba::wakeup(void) {
  comm_ -> setRts(0);
  sleep_for_sec(0.1);
  comm_ -> setRts(1);
  sleep_for_sec(COMMAND_WAIT);
}

void Roomba::startup(void) {
  sendOpCode(OC_START);
  sendOpCode(OC_CONTROL);
}

void Roomba::powerOff() {
  sendOpCode(OC_POWER);
  sleep_for_sec(COMMAND_WAIT);
}

void Roomba::clean() {
  sendOpCode(OC_CLEAN);
  sleep_for_sec(COMMAND_WAIT);
}

void Roomba::safe() {
  sendOpCode(OC_SAFE);
  sleep_for_sec(COMMAND_WAIT);
}

void Roomba::full() {
  sendOpCode(OC_FULL);
  sleep_for_sec(COMMAND_WAIT);
}

void Roomba::spot() {
  sendOpCode(OC_SPOT);
  sleep_for_sec(COMMAND_WAIT);
}

void Roomba::max() {
  sendOpCode(OC_MAX);
  sleep_for_sec(COMMAND_WAIT);
}

void Roomba::dock() {
  const uint8 seq[] = {OC_BUTTONS, BB_DOCK};
  comm_ -> write(seq,2);
  sleep_for_sec(COMMAND_WAIT);
}

void Roomba::setMotorState(MOTOR_STATE_BITS state) {
  const uint8 seq[] = {OC_MOTORS, state};
  comm_ -> write(seq,2);
  sleep_for_sec(COMMAND_WAIT);
}

void Roomba::seekDock() {
  const uint8 seq[] = {OC_SEEK_DOCK};
  comm_ -> write(seq,1);
  sleep_for_sec(COMMAND_WAIT);
}

void Roomba::drive(short velocity, short radius) {
  uint8 vhi = (uint8)(velocity >> 8);
  uint8 vlo = (uint8)(velocity & 0xff);
  uint8 rhi = (uint8)(radius >> 8);
  uint8 rlo = (uint8)(radius & 0xff);

  const uint8 seq[] = {OC_DRIVE, vhi, vlo, rhi, rlo};
  comm_ -> write(seq, 5);
  sleep_for_sec(COMMAND_WAIT);
}

void Roomba::driveDirect(float velocity, float yawrate) {
  short right = 1000 * (velocity + 0.5 * 0.235 * yawrate);
  short left = 1000 * (velocity - 0.5 * 0.235 * yawrate);

  uint8 rhi = (uint8)(right >> 8);
  uint8 rlo = (uint8)(right & 0xff);
  uint8 lhi = (uint8)(left  >> 8);
  uint8 llo = (uint8)(left  & 0xff);

  const uint8 seq[] = {OC_DRIVE_DIRECT, rhi, rlo, lhi, llo};
  comm_ -> write(seq, 5);
  sleep_for_sec(COMMAND_WAIT);
}

void Roomba::drivePWM(int right_pwm, int left_pwm) {
  short right = 255.0 / 100.0 * right_pwm;
  short left = 255.0 / 100.0 * left_pwm;

  uint8 rhi = (uint8)(right >> 8);
  uint8 rlo = (uint8)(right & 0xff);
  uint8 lhi = (uint8)(left >> 8);
  uint8 llo = (uint8)(left & 0xff);

  const uint8 seq[] = {OC_DRIVE_PWM, rhi, rlo, lhi, llo};
  comm_ -> write(seq,5);
  sleep_for_sec(COMMAND_WAIT);
}

float Roomba::velToPWM(float velocity) {
  if(velocity > 0)
    return 173.38 * velocity + 14.55; //ff para
  else if(velocity<0)
    return 206.00 * velocity - 13.25; //ff para
  else
    return 0;
}

void Roomba::song(int song_number, int song_length) {
  const uint8 mode_seq[] = {OC_SAFE};
  comm_ -> write(mode_seq, 1);
  sleep_for_sec(COMMAND_WAIT);

  const uint8 command_seq[] = {OC_SONG, song_number, song_length, 60, 126};
  comm_ -> write(command_seq,2*song_length+3);
  sleep_for_sec(COMMAND_WAIT);
}

void Roomba::playing(int song_number) {
  const uint8 command_seq[] = {OC_PLAY, song_number};
  comm_ -> write(command_seq,2);
  sleep_for_sec(COMMAND_WAIT);
}

void Roomba::updateSensorState() {
  boost::mutex::scoped_lock(sensor_mutex_);
  uint8 raw_state[80];

  uint8 dataByte = ALL_PACKET;
  sendOpCode(OC_SENSORS, &dataByte, 1);
  int res = comm_ -> read(raw_state, 80);
  if (80 == res) {
    convertState(raw_state, sensor_);
  } else {
    //std::string err_msg(__func__); err_msg += ":Failed to receive sensor state.";
    //throw std::runtime_error(err_msg.c_str());
  }
}

RoombaSensors Roomba::getSensorState() const {
  boost::mutex::scoped_lock(sensor_mutex_);
  return sensor_;
}

float Roomba::getCtrlLinearX() {
  return ctrl_.cntl.linear.x;
}

float Roomba::getCtrlAngleZ() {
  return ctrl_.cntl.angular.z;
}

void Roomba::sendCtrl(const roomba_500driver_meiji::RoombaCtrlConstPtr& msg) {
  boost::mutex::scoped_lock(ctrl_mutex_);
  ctrl_ = *msg;
  switch(msg->mode){
  case roomba_500driver_meiji::RoombaCtrl::SPOT:
    spot();
    break;
  case roomba_500driver_meiji::RoombaCtrl::SAFE:
    safe();
    break;
  case roomba_500driver_meiji::RoombaCtrl::CLEAN:
    clean();
    break;
  case roomba_500driver_meiji::RoombaCtrl::POWER:
    powerOff();
    break;
  case roomba_500driver_meiji::RoombaCtrl::WAKEUP:
    wakeup();
    startup();
    break;
  case roomba_500driver_meiji::RoombaCtrl::FULL:
    full();
    break;
  case roomba_500driver_meiji::RoombaCtrl::MAX:
    max();
    break;
  case roomba_500driver_meiji::RoombaCtrl::DOCK:
    dock();
    break;
  case roomba_500driver_meiji::RoombaCtrl::MOTORS:
    setMotorState((roombaC2::MOTOR_STATE_BITS)
		  (roombaC2::MB_MAIN_BRUSH |
		   roombaC2::MB_SIDE_BRUSH |
		   roombaC2::MB_VACUUM));
    break;
  case roomba_500driver_meiji::RoombaCtrl::MOTORS_OFF:
    setMotorState((roombaC2::MOTOR_STATE_BITS)(0));
    break;
  case roomba_500driver_meiji::RoombaCtrl::DRIVE_DIRECT:
    driveDirect(msg->cntl.linear.x, msg->cntl.angular.z);
    break;
  case roomba_500driver_meiji::RoombaCtrl::DRIVE_PWM:
    drivePWM(msg->r_pwm, msg->l_pwm);
    break;
  case roomba_500driver_meiji::RoombaCtrl::SONG:
    safe();
    song(1,1);
    playing(1);
    break;
  case roomba_500driver_meiji::RoombaCtrl::DRIVE:
  default:
    drive(msg->velocity, msg->radius);
  }
}

void Roomba::convertState(const uint8 raw_sensor[80], RoombaSensors &sensor) {
  // Bumps and Wheel Drops
  sensor.bumps_wheeldrops.bump_right      = (raw_sensor[0] & BUMP_RIGHT);
  sensor.bumps_wheeldrops.bump_left       = (raw_sensor[0] & BUMP_LEFT);
  sensor.bumps_wheeldrops.wheeldrop_right = (raw_sensor[0] & WHEELDROP_RIGHT);
  sensor.bumps_wheeldrops.wheeldrop_left  = (raw_sensor[0] & WHEELDROP_LEFT);
  // Wall
  sensor.wall.wall                        = (raw_sensor[1] & 0x01);
  // Cliff
  sensor.cliff.left                       = (raw_sensor[2] & 0x01);
  sensor.cliff.front_left                 = (raw_sensor[3] & 0x01);
  sensor.cliff.front_right                = (raw_sensor[4] & 0x01);
  sensor.cliff.right                      = (raw_sensor[5] & 0x01);
  // Vertial Wall
  sensor.wall.vwall                       = (raw_sensor[6] & 0x01);
  // Wheel Overcurrents
  sensor.wheel_overcurrents.left_wheel    = (raw_sensor[7] & LEFT_WHEEL);
  sensor.wheel_overcurrents.right_wheel   = (raw_sensor[7] & RIGHT_WHEEL);
  sensor.wheel_overcurrents.main_brush    = (raw_sensor[7] & MAIN_BRUSH);
  sensor.wheel_overcurrents.side_brush    = (raw_sensor[7] & SIDE_BRUSH);
  // Dirt Detect
  sensor.dirt_detect                      = raw_sensor[8];
  // Unused Byte
  // raw_sensor[9] is unused byte
  // IR Opcodes
  sensor.ir_opcodes.omni                  = raw_sensor[10];
  sensor.ir_opcodes.left                  = raw_sensor[69]; // <- Attention
  sensor.ir_opcodes.right                 = raw_sensor[70]; // <- It's confusing.
  // Buttons
  sensor.button.clean                     = (raw_sensor[11] & BB_CLEAN);
  sensor.button.spot                      = (raw_sensor[11] & BB_SPOT);
  sensor.button.dock                      = (raw_sensor[11] & BB_DOCK);
  sensor.button.minute                    = (raw_sensor[11] & BB_MINUTE);
  sensor.button.hour                      = (raw_sensor[11] & BB_HOUR);
  sensor.button.day                       = (raw_sensor[11] & BB_DAY);
  sensor.button.schedule                  = (raw_sensor[11] & BB_SCHEDULE);
  sensor.button.clock                     = (raw_sensor[11] & BB_CLOCK);
  // Traveled distance and angle
  sensor.travel.distance                  = *((int16*) (raw_sensor + 12));
  sensor.travel.angle                     = *((int16*) (raw_sensor + 14));
  // Battery Charging sensor, Voltage, Current, Temperature, Charge, Capacity
  sensor.battery.charging_state           = raw_sensor[16];
  sensor.battery.voltage                  = *((uint16*) (raw_sensor + 17));
  sensor.battery.current                  = *((int16*)  (raw_sensor + 19));
  sensor.battery.temperature              = *((int8*)   (raw_sensor + 21));
  sensor.battery.charge                   = *((uint16*) (raw_sensor + 22));
  sensor.battery.capacity                 = *((uint16*) (raw_sensor + 24));
  // Wall Signal Strength
  sensor.wall.wall_signal                 = *((uint16*) (raw_sensor + 26));
  // Cliff Signal Strength
  sensor.cliff.left_signal                = *((uint16*) (raw_sensor + 28));
  sensor.cliff.front_left_signal          = *((uint16*) (raw_sensor + 30));
  sensor.cliff.front_right_signal         = *((uint16*) (raw_sensor + 32));
  sensor.cliff.right_signal               = *((uint16*) (raw_sensor + 34));
  // Unused Byte
  // raw_sensor[36, 37, 38] is unused byte
  // Charging sources availability
  sensor.charging_source.home_base        = (raw_sensor[39] & HOME_BASE);
  sensor.charging_source.internal_charger = (raw_sensor[39] & INTERNAL_CHARGER);
  // OI Mode
  sensor.oi_mode                          = raw_sensor[40];
  // Song
  sensor.song.number                      = raw_sensor[41];
  sensor.song.playing                     = raw_sensor[42];
  // #Stream raw_sensorets
  sensor.stream_packets                   = raw_sensor[43];
  // Requested Velocity and radius
  sensor.request.velocity                 = *((int16*) (raw_sensor + 44));
  sensor.request.radius                   = *((int16*) (raw_sensor + 46));
  sensor.request.right_velocity           = *((int16*) (raw_sensor + 48));
  sensor.request.left_velocity            = *((int16*) (raw_sensor + 50));
  // Encoder counts
  sensor.encoder_counts.left              = *((uint16*) (raw_sensor + 52));
  sensor.encoder_counts.right             = *((uint16*) (raw_sensor + 54));
  // Light Bumper
  sensor.light_bumper.left                = (raw_sensor[56] & LT_BUMPER_LEFT);
  sensor.light_bumper.front_left          = (raw_sensor[56] & LT_BUMPER_FRONT_LEFT);
  sensor.light_bumper.center_left         = (raw_sensor[56] & LT_BUMPER_CENTER_LEFT);
  sensor.light_bumper.center_right        = (raw_sensor[56] & LT_BUMPER_CENTER_RIGHT);
  sensor.light_bumper.front_right         = (raw_sensor[56] & LT_BUMPER_FRONT_RIGHT);
  sensor.light_bumper.right               = (raw_sensor[56] & LT_BUMPER_RIGHT);
  // Light Bumper Signal Strength
  sensor.light_bumper.left_signal         = *((uint16*) (raw_sensor + 57));
  sensor.light_bumper.front_left_signal   = *((uint16*) (raw_sensor + 59));
  sensor.light_bumper.center_left_signal  = *((uint16*) (raw_sensor + 61));
  sensor.light_bumper.center_right_signal = *((uint16*) (raw_sensor + 63));
  sensor.light_bumper.front_right_signal  = *((uint16*) (raw_sensor + 65));
  sensor.light_bumper.right_signal        = *((uint16*) (raw_sensor + 67));
  // Motor Current
  sensor.motor_current.left_wheel         = *((int16*) (raw_sensor + 71));
  sensor.motor_current.right_wheel        = *((int16*) (raw_sensor + 73));
  sensor.motor_current.main_brush         = *((int16*) (raw_sensor + 75));
  sensor.motor_current.side_brush         = *((int16*) (raw_sensor + 77));
  // Stasis
  sensor.stasis                           =  raw_sensor[79];
}

#include "InternalCommands.h"

#include "../../ESPEasy_common.h"
#include "../../ESPEasy_fdwdecl.h"
#include "../../ESPEasy_Log.h"
#include "../../_Plugin_Helper.h"
#include "../Globals/Settings.h"

#ifdef USES_BLYNK
# include "../Commands/Blynk.h"
# include "../Commands/Blynk_c015.h"
#endif // ifdef USES_BLYNK

#include "../Commands/Common.h"
#include "../Commands/Controller.h"
#include "../Commands/Diagnostic.h"
#include "../Commands/GPIO.h"
#include "../Commands/HTTP.h"
#include "../Commands/i2c.h"

#ifdef USES_MQTT
# include "../Commands/MQTT.h"
#endif // USES_MQTT

#include "../Commands/Networks.h"
#include "../Commands/Notifications.h"
#include "../Commands/RTC.h"
#include "../Commands/Rules.h"
#include "../Commands/SDCARD.h"
#include "../Commands/Settings.h"
#include "../Commands/System.h"
#include "../Commands/Tasks.h"
#include "../Commands/Time.h"
#include "../Commands/Timer.h"
#include "../Commands/UPD.h"
#include "../Commands/wd.h"
#include "../Commands/WiFi.h"

#include "../Helpers/Misc.h"
#include "../Helpers/StringConverter.h"
#include "../Helpers/StringParser.h"


bool checkNrArguments(const char *cmd, const char *Line, int nrArguments) {
  if (nrArguments < 0) { return true; }

  // 0 arguments means argument on pos1 is valid (the command) and argpos 2 should not be there.
  if (HasArgv(Line, nrArguments + 2)) {
    if (loglevelActiveFor(LOG_LEVEL_ERROR)) {
      String log;
      log.reserve(128);
      log += F("Too many arguments: cmd=");
      log += cmd;

      if (nrArguments < 1) {
        log += Line;
      } else {
        // Check for one more argument than allowed, since we apparently have one.
        bool done = false;
        int  i    = 1;

        while (!done) {
          String parameter;

          if (i == nrArguments) {
            parameter = tolerantParseStringKeepCase(Line, i + 1);
            done      = true;
          } else {
            parameter = parseStringKeepCase(Line, i + 1);
          }
          done = parameter.length() == 0;

          if (!done) {
            if (i <= nrArguments) {
              if (Settings.TolerantLastArgParse() && (i == nrArguments)) {
                log += F(" (fixed)");
              }
              log += F(" Arg");
            } else {
              log += F(" ExtraArg");
            }
            log += String(i);
            log += '=';
            log += parameter;
          }
          ++i;
        }
      }
      log += F(" lineLength=");
      log += strlen(Line);
      addLog(LOG_LEVEL_ERROR, log);
      log  = F("Line: _");
      log += Line;
      log += '_';
      addLog(LOG_LEVEL_ERROR, log);

      if (!Settings.TolerantLastArgParse()) {
        log = F("Command not executed!");
      } else {
        log = F("Command executed, but may fail.");
      }
      log += F(" See: https://github.com/letscontrolit/ESPEasy/issues/2724");
      addLog(LOG_LEVEL_ERROR, log);
    }

    if (Settings.TolerantLastArgParse()) {
      return true;
    }
    return false;
  }
  return true;
}

bool checkSourceFlags(EventValueSource::Enum source, EventValueSourceGroup::Enum group) {
  if (EventValueSource::partOfGroup(source, group)) {
    return true;
  }
  addLog(LOG_LEVEL_ERROR, return_incorrect_source());
  return false;
}

bool do_command_case(const String              & cmd_lc,
                     const char                 *cmd,
                     struct EventStruct         *event,
                     const char                 *line,
                     String                    & status,
                     const String              & cmd_test,
                     command_function            pFunc,
                     int                         nrArguments,
                     EventValueSourceGroup::Enum group,
                     bool                      & retval)
{
  if (cmd_lc.equals(cmd_test)) {
    if (!checkSourceFlags(event->Source, group)) {
      status = return_incorrect_source();
      return false;
    } else if (!checkNrArguments(cmd, line, nrArguments)) {
      status = return_incorrect_nr_arguments();
      retval = false;
    } else  {
      status = pFunc(event, line);
      retval = true;
    }
    return true; // Command is handled
  }
  return false;
}

bool executeInternalCommand(const char *cmd, struct EventStruct *event, const char *line, String& status)
{
  String cmd_lc;

  cmd_lc = cmd;
  cmd_lc.toLowerCase();
  bool retval;

  // Simple macro to match command to function call.

  // EventValueSourceGroup::Enum::ALL
  #define COMMAND_CASE_A(S, C, NARGS) \
  if (do_command_case(cmd_lc, cmd, event, line, status, F(S), &C, NARGS, EventValueSourceGroup::Enum::ALL, retval)) { return retval; }

  // EventValueSourceGroup::Enum::RESTRICTED
  #define COMMAND_CASE_R(S, C, NARGS) \
  if (do_command_case(cmd_lc, cmd, event, line, status, F(S), &C, NARGS, EventValueSourceGroup::Enum::RESTRICTED, retval)) { return retval; }

  // FIXME TD-er: Should we execute command when number of arguments is wrong?

  // FIXME TD-er: must determine nr arguments where NARGS is set to -1

  switch (cmd_lc[0]) {
    case 'a': {
      COMMAND_CASE_A("accessinfo", Command_AccessInfo_Ls,       0); // Network Command
      COMMAND_CASE_A("asyncevent", Command_Rules_Async_Events, -1); // Rule.h
      break;
    }
    case 'b': {
    #ifndef BUILD_NO_DIAGNOSTIC_COMMANDS
      COMMAND_CASE_R("background", Command_Background, 1); // Diagnostic.h
    #endif // ifndef BUILD_NO_DIAGNOSTIC_COMMANDS
    #ifdef USES_C012
      COMMAND_CASE_A("blynkget", Command_Blynk_Get, -1);
    #endif // ifdef USES_C012
    #ifdef USES_C015
      COMMAND_CASE_R("blynkset", Command_Blynk_Set, -1);
    #endif // ifdef USES_C015
      COMMAND_CASE_A("build", Command_Settings_Build, 1);      // Settings.h
      break;
    }
    case 'c': {
      COMMAND_CASE_R( "clearaccessblock", Command_AccessInfo_Clear,   0); // Network Command
      COMMAND_CASE_R(      "clearrtcram", Command_RTC_Clear,          0); // RTC.h
      COMMAND_CASE_R(           "config", Command_Task_RemoteConfig, -1); // Tasks.h
      COMMAND_CASE_R("controllerdisable", Command_Controller_Disable, 1); // Controller.h
      COMMAND_CASE_R( "controllerenable", Command_Controller_Enable,  1); // Controller.h

      break;
    }
    case 'd': {
      COMMAND_CASE_R( "datetime", Command_DateTime,         2); // Time.h
      COMMAND_CASE_R(    "debug", Command_Debug,            1); // Diagnostic.h
      COMMAND_CASE_R("deepsleep", Command_System_deepSleep, 1); // System.h
      COMMAND_CASE_R(    "delay", Command_Delay,            1); // Timers.h
      COMMAND_CASE_R(      "dns", Command_DNS,              1); // Network Command
      COMMAND_CASE_R(      "dst", Command_DST,              1); // Time.h
      break;
    }
    case 'e': {
    #ifdef HAS_ETHERNET
      COMMAND_CASE_R(   "ethphyadr", Command_ETH_Phy_Addr,   1); // Network Command
      COMMAND_CASE_R(   "ethpinmdc", Command_ETH_Pin_mdc,    1); // Network Command
      COMMAND_CASE_R(  "ethpinmdio", Command_ETH_Pin_mdio,   1); // Network Command
      COMMAND_CASE_R( "ethpinpower", Command_ETH_Pin_power,  1); // Network Command
      COMMAND_CASE_R(  "ethphytype", Command_ETH_Phy_Type,   1); // Network Command
      COMMAND_CASE_R("ethclockmode", Command_ETH_Clock_Mode, 1); // Network Command
      COMMAND_CASE_R(       "ethip", Command_ETH_IP,         1); // Network Command
      COMMAND_CASE_R(  "ethgateway", Command_ETH_Gateway,    1); // Network Command
      COMMAND_CASE_R(   "ethsubnet", Command_ETH_Subnet,     1); // Network Command  
      COMMAND_CASE_R(      "ethdns", Command_ETH_DNS,        1); // Network Command
      COMMAND_CASE_R( "ethwifimode", Command_ETH_Wifi_Mode,  1); // Network Command
    #endif // HAS_ETHERNET
      COMMAND_CASE_R("erasesdkwifi", Command_WiFi_Erase,     0); // WiFi.h
      COMMAND_CASE_A(       "event", Command_Rules_Events,  -1); // Rule.h
      COMMAND_CASE_A("executerules", Command_Rules_Execute, -1); // Rule.h
      break;
    }
    case 'g': {
      COMMAND_CASE_R("gateway", Command_Gateway, 1);        // Network Command
      COMMAND_CASE_A(      "gpio", Command_GPIO,        2); // Gpio.h
      COMMAND_CASE_A("gpiotoggle", Command_GPIO_Toggle, 1); // Gpio.h
      break;
    }
    case 'i': {
      COMMAND_CASE_R("i2cscanner", Command_i2c_Scanner, -1); // i2c.h
      COMMAND_CASE_R(        "ip", Command_IP,           1); // Network Command
      break;
    }
    case 'j': {
      #ifndef BUILD_NO_DIAGNOSTIC_COMMANDS
      COMMAND_CASE_A("jsonportstatus", Command_JSONPortStatus, -1); // Diagnostic.h
      #endif // ifndef BUILD_NO_DIAGNOSTIC_COMMANDS
      break;
    }
    case 'l': {
      COMMAND_CASE_A(          "let", Command_Rules_Let,     2); // Rules.h
      COMMAND_CASE_A(         "load", Command_Settings_Load, 0); // Settings.h
      COMMAND_CASE_A(     "logentry", Command_logentry,      1); // Diagnostic.h
      COMMAND_CASE_A(   "looptimerset", Command_Loop_Timer_Set,    3); // Timers.h
      COMMAND_CASE_A("looptimerset_ms", Command_Loop_Timer_Set_ms, 3); // Timers.h
      COMMAND_CASE_A(    "longpulse", Command_GPIO_LongPulse,   3);    // GPIO.h
      COMMAND_CASE_A( "longpulse_ms", Command_GPIO_LongPulse_Ms,3);    // GPIO.h
    #ifndef BUILD_NO_DIAGNOSTIC_COMMANDS
      COMMAND_CASE_A("logportstatus", Command_logPortStatus,    0); // Diagnostic.h
      COMMAND_CASE_A(       "lowmem", Command_Lowmem,           0); // Diagnostic.h
    #endif // ifndef BUILD_NO_DIAGNOSTIC_COMMANDS
      break;
    }
    case 'm': {
      if (cmd_lc[1] == 'c') {
        COMMAND_CASE_A(        "mcpgpio", Command_GPIO,              2); // Gpio.h
        COMMAND_CASE_A(  "mcpgpiotoggle", Command_GPIO_Toggle,       1); // Gpio.h
        COMMAND_CASE_A(   "mcplongpulse", Command_GPIO_LongPulse,    3); // GPIO.h
        COMMAND_CASE_A("mcplongpulse_ms", Command_GPIO_LongPulse_Ms, 3); // GPIO.h
        COMMAND_CASE_A(       "mcppulse", Command_GPIO_Pulse,        3); // GPIO.h
      }
      COMMAND_CASE_A(      "monitor", Command_GPIO_Monitor,   2); // GPIO.h
    #ifndef BUILD_NO_DIAGNOSTIC_COMMANDS
      COMMAND_CASE_A(       "malloc", Command_Malloc,         1);        // Diagnostic.h
      COMMAND_CASE_A(      "meminfo", Command_MemInfo,        0);        // Diagnostic.h
      COMMAND_CASE_A("meminfodetail", Command_MemInfo_detail, 0);        // Diagnostic.h
    #endif // ifndef BUILD_NO_DIAGNOSTIC_COMMANDS

      break;
    }
    case 'n': {
      COMMAND_CASE_R(   "name", Command_Settings_Name,        1); // Settings.h
      COMMAND_CASE_R("nosleep", Command_System_NoSleep,       1); // System.h
      COMMAND_CASE_R( "notify", Command_Notifications_Notify, 2); // Notifications.h
      COMMAND_CASE_R("ntphost", Command_NTPHost,              1); // Time.h
      break;
    }
    case 'p': {
      if (cmd_lc[1] == 'c') {
        COMMAND_CASE_A(        "pcfgpio", Command_GPIO,              2); // Gpio.h
        COMMAND_CASE_A(  "pcfgpiotoggle", Command_GPIO_Toggle,       1); // Gpio.h
        COMMAND_CASE_A(   "pcflongpulse", Command_GPIO_LongPulse,    3); // GPIO.h
        COMMAND_CASE_A("pcflongpulse_ms", Command_GPIO_LongPulse_Ms, 3); // GPIO.h
        COMMAND_CASE_A(       "pcfpulse", Command_GPIO_Pulse,        3); // GPIO.h
      }
      COMMAND_CASE_R("password", Command_Settings_Password, 1);          // Settings.h
      COMMAND_CASE_A(   "pulse", Command_GPIO_Pulse,        3); // GPIO.h
#ifdef USES_MQTT
      COMMAND_CASE_A("publish", Command_MQTT_Publish, 2);                // MQTT.h
#endif // USES_MQTT
      break;
    }
    case 'r': {
      COMMAND_CASE_A("reboot", Command_System_Reboot, 0);                              // System.h
      COMMAND_CASE_R("reset", Command_Settings_Reset, 0);                              // Settings.h
      COMMAND_CASE_A("resetflashwritecounter", Command_RTC_resetFlashWriteCounter, 0); // RTC.h
      COMMAND_CASE_A(               "restart", Command_System_Restart,             0); // System.h
      COMMAND_CASE_A(                 "rules", Command_Rules_UseRules,             1); // Rule.h
      break;
    }
    case 's': {
      COMMAND_CASE_R(    "save", Command_Settings_Save, 0); // Settings.h
    #ifdef FEATURE_SD
      COMMAND_CASE_R(  "sdcard", Command_SD_LS,         0); // SDCARDS.h
      COMMAND_CASE_R("sdremove", Command_SD_Remove,     1); // SDCARDS.h
    #endif // ifdef FEATURE_SD

      if (cmd_lc[1] == 'e') {
        COMMAND_CASE_A(    "sendto", Command_UPD_SendTo,      2); // UDP.h    // FIXME TD-er: These send commands, can we determine the nr
                                                                  // of
                                                                  // arguments?
        COMMAND_CASE_A("sendtohttp", Command_HTTP_SendToHTTP, 3); // HTTP.h
        COMMAND_CASE_A( "sendtoudp", Command_UDP_SendToUPD,   3); // UDP.h
    #ifndef BUILD_NO_DIAGNOSTIC_COMMANDS
        COMMAND_CASE_R("serialfloat", Command_SerialFloat,    0); // Diagnostic.h
    #endif // ifndef BUILD_NO_DIAGNOSTIC_COMMANDS
        COMMAND_CASE_R(   "settings", Command_Settings_Print, 0); // Settings.h
      }
      COMMAND_CASE_A("status", Command_GPIO_Status,          2); // GPIO.h
      COMMAND_CASE_R("subnet", Command_Subnet, 1);                // Network Command
    #ifdef USES_MQTT
      COMMAND_CASE_A("subscribe", Command_MQTT_Subscribe, 1);     // MQTT.h
    #endif // USES_MQTT
    #ifndef BUILD_NO_DIAGNOSTIC_COMMANDS
      COMMAND_CASE_A(  "sysload", Command_SysLoad,        0);     // Diagnostic.h
    #endif // ifndef BUILD_NO_DIAGNOSTIC_COMMANDS
      break;
    }
    case 't': {
      if (cmd_lc[1] == 'a') {
        COMMAND_CASE_R(   "taskclear", Command_Task_Clear,    1);             // Tasks.h
        COMMAND_CASE_R("taskclearall", Command_Task_ClearAll, 0);             // Tasks.h
        COMMAND_CASE_R( "taskdisable", Command_Task_Disable,  1);             // Tasks.h
        COMMAND_CASE_R(  "taskenable", Command_Task_Enable,   1);             // Tasks.h
        COMMAND_CASE_A(           "taskrun", Command_Task_Run,            1); // Tasks.h
        COMMAND_CASE_A(      "taskvalueset", Command_Task_ValueSet,       3); // Tasks.h
        COMMAND_CASE_A(   "taskvaluetoggle", Command_Task_ValueToggle,    2); // Tasks.h
        COMMAND_CASE_A("taskvaluesetandrun", Command_Task_ValueSetAndRun, 3); // Tasks.h
      } else if (cmd_lc[1] == 'i') {
        COMMAND_CASE_A( "timerpause", Command_Timer_Pause,  1);               // Timers.h
        COMMAND_CASE_A("timerresume", Command_Timer_Resume, 1);               // Timers.h
        COMMAND_CASE_A(   "timerset", Command_Timer_Set,    2);               // Timers.h
        COMMAND_CASE_A("timerset_ms", Command_Timer_Set_ms, 2); // Timers.h
        COMMAND_CASE_R("timezone", Command_TimeZone, 1);                      // Time.h
      }      
      break;
    }
    case 'u': {
      COMMAND_CASE_R("udpport", Command_UDP_Port,      1);    // UDP.h
      COMMAND_CASE_R("udptest", Command_UDP_Test,      2);    // UDP.h
      COMMAND_CASE_R(   "unit", Command_Settings_Unit, 1);    // Settings.h
      COMMAND_CASE_A("unmonitor", Command_GPIO_UnMonitor, 2); // GPIO.h
      COMMAND_CASE_R("usentp", Command_useNTP, 1);            // Time.h
      break;
    }
    case 'w': {
      COMMAND_CASE_R("wdconfig", Command_WD_Config, 3);               // WD.h
      COMMAND_CASE_R(  "wdread", Command_WD_Read,   2);               // WD.h

      if (cmd_lc[1] == 'i') {
        COMMAND_CASE_R(    "wifiapmode", Command_Wifi_APMode,     0); // WiFi.h
        COMMAND_CASE_A(   "wificonnect", Command_Wifi_Connect,    0); // WiFi.h
        COMMAND_CASE_A("wifidisconnect", Command_Wifi_Disconnect, 0); // WiFi.h
        COMMAND_CASE_R(       "wifikey", Command_Wifi_Key,        1); // WiFi.h
        COMMAND_CASE_R(      "wifikey2", Command_Wifi_Key2,       1); // WiFi.h
        COMMAND_CASE_R(      "wifimode", Command_Wifi_Mode,       1); // WiFi.h
        COMMAND_CASE_R(      "wifiscan", Command_Wifi_Scan,       0); // WiFi.h
        COMMAND_CASE_R(      "wifissid", Command_Wifi_SSID,       1); // WiFi.h
        COMMAND_CASE_R(     "wifissid2", Command_Wifi_SSID2,      1); // WiFi.h
        COMMAND_CASE_R(   "wifistamode", Command_Wifi_STAMode,    0); // WiFi.h
      }
      break;
    }
    default:
      break;
  }

  #undef COMMAND_CASE_R
  #undef COMMAND_CASE_A
  return false;
}

// Execute command which may be plugin or internal commands
bool ExecuteCommand_all(EventValueSource::Enum source, const char *Line)
{
  return ExecuteCommand(INVALID_TASK_INDEX, source, Line, true, true, false);
}

bool ExecuteCommand_all_config(EventValueSource::Enum source, const char *Line)
{
  return ExecuteCommand(INVALID_TASK_INDEX, source, Line, true, true, true);
}

bool ExecuteCommand_plugin_config(EventValueSource::Enum source, const char *Line)
{
  return ExecuteCommand(INVALID_TASK_INDEX, source, Line, true, false, true);
}

bool ExecuteCommand_all_config_eventOnly(EventValueSource::Enum source, const char *Line)
{
  bool tryInternal = false;
  {
    String cmd;

    if (GetArgv(Line, cmd, 1)) {
      tryInternal = cmd.equalsIgnoreCase(F("event"));
    }
  }

  return ExecuteCommand(INVALID_TASK_INDEX, source, Line, true, tryInternal, true);
}

bool ExecuteCommand_internal(EventValueSource::Enum source, const char *Line)
{
  return ExecuteCommand(INVALID_TASK_INDEX, source, Line, false, true, false);
}

bool ExecuteCommand_plugin(EventValueSource::Enum source, const char *Line)
{
  return ExecuteCommand(INVALID_TASK_INDEX, source, Line, true, false, false);
}

bool ExecuteCommand_plugin(taskIndex_t taskIndex, EventValueSource::Enum source, const char *Line)
{
  return ExecuteCommand(taskIndex, source, Line, true, false, false);
}

bool ExecuteCommand(taskIndex_t            taskIndex,
                    EventValueSource::Enum source,
                    const char            *Line,
                    bool                   tryPlugin,
                    bool                   tryInternal,
                    bool                   tryRemoteConfig)
{
  checkRAM(F("ExecuteCommand"));
  String cmd;

  if (!GetArgv(Line, cmd, 1)) {
    SendStatus(source, return_command_failed());
    return false;
  }

  if (tryInternal) {
    // Small optimization for events, which happen frequently
    // FIXME TD-er: Make quick check to see if a command is an internal command, so we don't need to try all
    if (cmd.equalsIgnoreCase(F("event"))) {
      tryPlugin       = false;
      tryRemoteConfig = false;
    }
  }

  struct EventStruct TempEvent;

  // FIXME TD-er: Not sure what happens now, but TaskIndex cannot always be set here
  // since commands can originate from anywhere.
  TempEvent.setTaskIndex(taskIndex);
  TempEvent.sensorType = getDeviceVTypeForTask(taskIndex);
  TempEvent.Source = source;

  String action(Line);
  action = parseTemplate(action); // parseTemplate before executing the command

  // Split the arguments into Par1...5 of the event.
  // Do not split it in executeInternalCommand, since that one will be called from the scheduler with pre-set events.
  // FIXME TD-er: Why call this for all commands? The CalculateParam function is quite heavy.
  parseCommandString(&TempEvent, action);

  // FIXME TD-er: This part seems a bit strange.
  // It can't schedule a call to PLUGIN_WRITE.
  // Maybe ExecuteCommand can be scheduled?
  delay(0);

  if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
    String log = F("Command: ");
    log += cmd;
    addLog(LOG_LEVEL_DEBUG, log);
#ifndef BUILD_NO_DEBUG
    addLog(LOG_LEVEL_DEBUG, Line); // for debug purposes add the whole line.
    String parameters;
    parameters.reserve(64);
    parameters += F("Par1: ");
    parameters += TempEvent.Par1;
    parameters += F(" Par2: ");
    parameters += TempEvent.Par2;
    parameters += F(" Par3: ");
    parameters += TempEvent.Par3;
    parameters += F(" Par4: ");
    parameters += TempEvent.Par4;
    parameters += F(" Par5: ");
    parameters += TempEvent.Par5;
    addLog(LOG_LEVEL_DEBUG, parameters);
#endif // ifndef BUILD_NO_DEBUG
  }


  if (tryInternal) {
    String status;
    bool   handled = executeInternalCommand(cmd.c_str(), &TempEvent, action.c_str(), status);

    if (status.length() > 0) {
      delay(0);
      SendStatus(source, status);
      delay(0);
    }

    if (handled) {
      return true;
    }
  }

  if (tryPlugin) {
    // Use a tmp string to call PLUGIN_WRITE, since PluginCall may inadvertenly
    // alter the string.
    String tmpAction(action);
    bool   handled = PluginCall(PLUGIN_WRITE, &TempEvent, tmpAction);

    if (!tmpAction.equals(action)) {
      if (loglevelActiveFor(LOG_LEVEL_ERROR)) {
        String log = F("PLUGIN_WRITE altered the string: ");
        log += action;
        log += F(" to: ");
        log += tmpAction;
        addLog(LOG_LEVEL_ERROR, log);
      }
    }

    if (handled) {
      SendStatus(source, return_command_success());
      return true;
    }
  }

  if (tryRemoteConfig) {
    if (remoteConfig(&TempEvent, action)) {
      SendStatus(source, return_command_success());
      return true;
    }
  }
  String errorUnknown = F("Command unknown: \"");
  errorUnknown += action;
  errorUnknown += '\"';
  addLog(LOG_LEVEL_INFO, errorUnknown);
  SendStatus(source, errorUnknown);
  delay(0);
  return false;
}

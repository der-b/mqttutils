/* Copyright 2014 Bernd Lehmann (der-b@der-b.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <mosquitto.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include "mqtt-player.h"
#include "config.h"
#include "log.h"

#define BUF_SIZE 256 

#define MSG_ARG_TYPE        0
#define MSG_ARG_TIME        1
#define MSG_ARG_QOS         2
#define MSG_ARG_RETAIN      3
#define MSG_ARG_PAYLOADLEN  4
#define MSG_ARG_TOPIC       5

struct _conf {
  #define CONF_DEFAULT_MQTT_CLIENT_ID     "mqtt-player"
  #define CONF_MAX_LENGTH_MQTT_CLIENT_ID  MOSQ_MQTT_ID_MAX_LENGTH
  char mqtt_client_id[CONF_MAX_LENGTH_MQTT_CLIENT_ID];

  #define CONF_DEFAULT_MQTT_BROKER     "localhost"
  #define CONF_MAX_LENGTH_MQTT_BROKER  256
  char mqtt_broker[CONF_MAX_LENGTH_MQTT_BROKER];

  #define CONF_DEFAULT_MQTT_TOPIC    "der-b/player"
  #define CONF_MAX_LENGTH_MQTT_TOPIC  256
  char mqtt_topic[CONF_MAX_LENGTH_MQTT_TOPIC];

  #define CONF_DEFAULT_LOG_FILE     ""
  #define CONF_MAX_LENGTH_LOG_FILE  256
  char log_file[CONF_MAX_LENGTH_LOG_FILE];

  #define CONF_DEFAULT_MQTT_PORT  1883
  int mqtt_port;

  #define CONF_DEFAULT_MQTT_CLEAN_SESSION  1
  char mqtt_clean_session;

  #define CONF_DEFAULT_MQTT_KEEPALIVE  60
  int mqtt_keepalive;

  #define CONF_DEFAULT_VERBOSE  0
  int verbose;

  #define CONF_DEFAULT_SEC   0
  #define CONF_DEFAULT_USEC  0
  struct timeval start_time;

  #define CONF_DEFAULT_RECORD_SEC   0
  #define CONF_DEFAULT_RECORD_USEC  0
  struct timeval record_start_time;

  #define CONF_DEFAULT_IGNORE_TIMING 0
  int ignore_timing;

  #define CONF_DEFAULT_REPEAT 0
  int repeat;

  struct mosquitto *mosq;
  FILE *fd;
  sigset_t sigset;
  struct timeval start;

} config;


/**
 * Initialize the configuration. Have to be called befor using the config variable.
 *
 * @return 0 on success, otherwise something else.
 */
int config_init() {
  strncpy(config.mqtt_client_id, CONF_DEFAULT_MQTT_CLIENT_ID, CONF_MAX_LENGTH_MQTT_CLIENT_ID);
  strncpy(config.mqtt_broker,    CONF_DEFAULT_MQTT_BROKER,    CONF_MAX_LENGTH_MQTT_BROKER);
  strncpy(config.mqtt_topic,     CONF_DEFAULT_MQTT_TOPIC,     CONF_MAX_LENGTH_MQTT_TOPIC);
  strncpy(config.log_file,       CONF_DEFAULT_LOG_FILE,       CONF_MAX_LENGTH_LOG_FILE);

  config.mqtt_port          = CONF_DEFAULT_MQTT_PORT;
  config.mqtt_clean_session = CONF_DEFAULT_MQTT_CLEAN_SESSION;
  config.mqtt_keepalive     = CONF_DEFAULT_MQTT_KEEPALIVE;
  config.verbose            = CONF_DEFAULT_VERBOSE;

  config.start_time.tv_sec  = CONF_DEFAULT_SEC;
  config.start_time.tv_usec = CONF_DEFAULT_USEC;
  
  config.record_start_time.tv_sec  = CONF_DEFAULT_RECORD_SEC;
  config.record_start_time.tv_usec = CONF_DEFAULT_RECORD_USEC;

  config.ignore_timing      = CONF_DEFAULT_IGNORE_TIMING;
  config.repeat             = CONF_DEFAULT_REPEAT;

  config.mosq = NULL;
  if( 0 > sigemptyset(&config.sigset) ) {
    CRIT("sigemptyset()");
  }

  if( 0 > sigaddset(&config.sigset, SIGINT) ) {
    CRIT("sigaddset()");
  }

  return 0;
}


/**
 * Prints the usage message of the program.
 *
 * @param progname Name of the program.
 */
void print_usage(char *progname) {
  printf("Usage: %s [options] <logfile>\n\n", progname);
  printf("Logfile is a file from wich the messages will be loaded.\n\n");
  printf("Options: \n");
  printf("-t --topic          MQTT topic where the program post messages about the player status.\n");
  printf("                    Default value: %s\n", CONF_DEFAULT_MQTT_TOPIC);
  printf("-b --broker         Hostname of the MQTT broker\n");
  printf("                    Default value: %s\n", CONF_DEFAULT_MQTT_BROKER);
  printf("-p --port           Port of the MQTT broker\n");
  printf("                    Default value: %d\n", CONF_DEFAULT_MQTT_PORT);
  printf("-c --client-id      MQTT client id of the this sensor\n");
  printf("                    Default value: %s\n", CONF_DEFAULT_MQTT_CLIENT_ID);
  printf("-x --clean-session  Defines wether the client connects with a clean session to the MQTT broker or not.\n");
  printf("                    Possible values: on|off\n");
  printf("                    Default value: %s\n", (CONF_DEFAULT_MQTT_CLEAN_SESSION)?("on"):("off"));
  printf("-k --keep-alive     Interval of the keepalive messages send to the broker in seconds.\n");
  printf("                    Default value: %d\n", CONF_DEFAULT_MQTT_KEEPALIVE);
  printf("-i --ignore-timing  Ignore the timing in the log file and replays it as fast as possible.\n");
  printf("-r --repeat         Repeat the log endlessly.\n");
  printf("-v --verbose        Print alot informations messages.\n");
  printf("-h --help           Print this help message.\n");
}


/**
 * Parse the commandline arguments. The first argument provided in argv is the 
 * program name.
 *
 * @param argc Number of arguments
 * @param argv Array of arguments. The first string is the program name.
 */
void parse_args(int argc, char **argv) {
  int i;

  for(i = 1; i < argc; i++) {
    
    // CLIENT ID
    if( !strcmp(argv[i], "-c") || !strcmp(argv[i], "--client-id") ) {

      if( ++i == argc ) {
        fprintf(stderr, "ERROR: Parameter %s given but no client id specified.\n", argv[i-1]);
	print_usage(*argv);
	exit(1);
      } else {
        strncpy(config.mqtt_client_id, argv[i], CONF_MAX_LENGTH_MQTT_CLIENT_ID);
      }

    // BROKER
    } else if( !strcmp(argv[i], "-b") || !strcmp(argv[i], "--broker") ) {

      if( ++i == argc ) {
        fprintf(stderr, "ERROR: Parameter %s given but no broker specified.\n", argv[i-1]);
	print_usage(*argv);
	exit(1);
      } else {
        strncpy(config.mqtt_broker, argv[i], CONF_MAX_LENGTH_MQTT_BROKER);
      }

    // TOPIC
    }else if( !strcmp(argv[i], "-t") || !strcmp(argv[i], "--topic") ) {
      if( ++i == argc ) {
        fprintf(stderr, "ERROR: Parameter %s given but no topic specified.\n", argv[i-1]);
	print_usage(*argv);
	exit(1);
      } else {
        strncpy(config.mqtt_topic, argv[i], CONF_MAX_LENGTH_MQTT_TOPIC);
      }

    // PORT
    } else if( !strcmp(argv[i], "-p") || !strcmp(argv[i], "--port") ) {
      if( ++i == argc ) {
        fprintf(stderr, "ERROR: Parameter %s given but no port specified.\n", argv[i-1]);
	print_usage(*argv);
	exit(1);
      } else {
        config.mqtt_port = atoi(argv[i]);
	if( 1 > config.mqtt_port || 65535 < config.mqtt_port ) {
	  fprintf(stderr, "ERROR: Invalid port given: %d\n", config.mqtt_port);
	  print_usage(*argv);
	  exit(1);
	}
      }

    // CLEAN SESSION
    } else if( !strcmp(argv[i], "-x") || !strcmp(argv[i], "--clean-session") ) {

      if( ++i == argc ) {
        fprintf(stderr, "ERROR: Parameter %s given but no value id specified.\n", argv[i-1]);
	print_usage(*argv);
	exit(1);
      } else {

        if( !strcmp(argv[i], "on") ) {
	  config.mqtt_clean_session = 1;
	} else if( !strcmp(argv[i], "off") ) {
	  config.mqtt_clean_session = 0;
	} else {
          fprintf(stderr, "ERROR: Invalid value for clean sessions.\n");
	  print_usage(*argv);
	  exit(1);
	}

      }

    // KEEP ALIVE
    } else if( !strcmp(argv[i], "-k") || !strcmp(argv[i], "--keep-alive") ) {
      if( ++i == argc ) {
        fprintf(stderr, "ERROR: Parameter %s given but no keep alive value specified.\n", argv[i-1]);
	print_usage(*argv);
	exit(1);
      } else {
        config.mqtt_keepalive = atoi(argv[i]);
      }

    // IGNORE TIMING
    } else if( !strcmp(argv[i], "-i") || !strcmp(argv[i], "--ignore-timing") ) {
      config.ignore_timing = 1;

    // REPEAT
    } else if( !strcmp(argv[i], "-r") || !strcmp(argv[i], "--repeat") ) {
      config.repeat = 1;

    // VERBOSE
    } else if( !strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose") ) {
      config.verbose = 1;

    // HELP
    } else if( !strcmp(argv[i], "-h") || !strcmp(argv[i], "--help") ) {
      print_usage(*argv);
      exit(0);
    
    }else if( '-' == *argv[i] ) {
        fprintf(stderr, "ERROR: Unknown parameter '%s'.\n", argv[i]);
	print_usage(*argv);
	exit(1);
      

    // FILE
    } else {
      strncpy(config.log_file, argv[i], CONF_MAX_LENGTH_LOG_FILE);
    }
  }
}

/**
 * Handles the signal from ctrl+C. Simple close the connection and close the file.
 */
void sig_handler(int sig) {
  if( SIGINT != sig ) {
    CRIT("Got unexpected signal.");
  }

  mosquitto_disconnect(config.mosq);

  mosquitto_destroy(config.mosq);

  mosquitto_lib_cleanup();

  fclose(config.fd);

  exit(0);
}

/**
 * Main!
 */
int main(int argc, char **argv) {
  struct sigaction sigact;
  char buf[BUF_SIZE];
  int qos, retain, len;
  struct timeval recv_time, now;
  char topic[CONF_MAX_LENGTH_MQTT_TOPIC];
  int i, ret;
  struct mqtt_player_status_msg status;

  if( config_init() ) {
    CRIT("Faild to initialize config.");
  }
  
  parse_args(argc, argv);

  if( !strlen(config.log_file) ) {
    fprintf(stderr, "ERROR: You have to provide a logfile.\n");
    print_usage(*argv);
    exit(1);
  }

  config.fd = fopen(config.log_file, "r");
  if( NULL == config.fd ) {
    CRIT("Could not open log file.");
  }

  memset(&sigact, 0, sizeof(struct sigaction));
  sigact.sa_handler = sig_handler;
  if( sigaction(SIGINT, &sigact, NULL) ) {
    CRIT("Could not initialize signal handler.");
  }
  
  mosquitto_lib_init();

  config.mosq = mosquitto_new(config.mqtt_client_id, config.mqtt_clean_session, NULL);
  if( NULL == config.mosq ) {
    CRIT("Could not create a mosquitto object.");
  }

  if( mosquitto_connect(config.mosq, config.mqtt_broker, config.mqtt_port, config.mqtt_keepalive) ) {
    CRIT("Could not connect MQTT broker.");
  }

  mosquitto_loop_start(config.mosq);

  do {

    if( gettimeofday(&config.start, NULL) ) {
      CRIT("Could not get time().");
    }

    if( 0 != fseek(config.fd, SEEK_SET, 0) ) {
      CRIT("lseek faild.");
    }

    // read file config
    while(1) {
      if( fscanf(config.fd, "cnf time: %ld.%ld\n", &config.record_start_time.tv_sec, &config.record_start_time.tv_usec) ) {
        if( config.verbose ) {
          printf("record time: %3ld", config.record_start_time.tv_sec);
          printf(".%06ld\n", config.record_start_time.tv_usec);
	}
      } else {
        break;
      }
    }

    if( config.verbose ) {
      printf("-- start playing --\n");
    }

    status.status = MQTT_PLAYER_BEGIN_PLAY;
    status.sec = hton64(config.record_start_time.tv_sec);
    status.usec = hton64(config.record_start_time.tv_usec);
    // post status
    mosquitto_publish(config.mosq, NULL, config.mqtt_topic, sizeof(struct mqtt_player_status_msg), &status, 2, 0);

    // read data
    while(1) {
      if( feof(config.fd) ) {
        break;
      }
  
      ret = fscanf(config.fd, "msg %ld.%ld %d %d %d %s\n", &recv_time.tv_sec, &recv_time.tv_usec, &qos, &retain, &len, topic);
      if( 0 == ret ) {
        break;
      }
  
      if( config.verbose ) {
        printf("time: %3ld", recv_time.tv_sec);
        printf(".%06ld ", recv_time.tv_usec);
        printf("qos: %d ", qos);
        printf("retain: %d ", retain);
        printf("len: %d ", len);
        printf("topic: %s\n", topic);
      }
  
      if( 0 > qos && 2 < qos ) {
        CRIT("Format error in '%s'.", config.log_file);
      }
  
      if( 0 > retain && 1 < retain ) {
        CRIT("Format error in '%s'.", config.log_file);
      }
  
      if( 0 < len ) {
        ret == fscanf(config.fd, "%02hhx", &buf[0]);
        if( 0 == ret ) {
          break;
        }
      }
  
      for( i = 1; i < len; i++ ) {
        ret == fscanf(config.fd, " %02hhx", &buf[i]);
        if( 0 == ret ) {
          break;
        }
      }
      ret == fscanf(config.fd, "\n");
      if( 0 == ret ) {
        break;
      }
  
      if( !config.ignore_timing ) {
        timeradd(&recv_time, &config.start, &recv_time);
  
        if( gettimeofday(&now, NULL) ) {
          CRIT("Could not get time.");
        }
  
        if( timercmp(&now, &recv_time, <) ) {
          timersub(&recv_time, &now, &recv_time);
          usleep(recv_time.tv_sec * 1000000 + recv_time.tv_usec);
        }
      }
  
      mosquitto_publish(config.mosq, NULL, topic, len, buf, qos, retain);
    }
    
  }while( config.repeat && feof(config.fd) );

  mosquitto_disconnect(config.mosq);
 
  mosquitto_loop_stop(config.mosq, false);

  mosquitto_destroy(config.mosq);

  mosquitto_lib_cleanup();

  fclose(config.fd);

  return 0;
}


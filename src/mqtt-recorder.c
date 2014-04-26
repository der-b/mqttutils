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
#include <sys/time.h>
#include <signal.h>
#include "config.h"
#include "log.h"

struct _conf {
  #define CONF_DEFAULT_MQTT_CLIENT_ID     "recorder"
  #define CONF_MAX_LENGTH_MQTT_CLIENT_ID  MOSQ_MQTT_ID_MAX_LENGTH
  char mqtt_client_id[CONF_MAX_LENGTH_MQTT_CLIENT_ID];

  #define CONF_DEFAULT_MQTT_BROKER     "localhost"
  #define CONF_MAX_LENGTH_MQTT_BROKER  256
  char mqtt_broker[CONF_MAX_LENGTH_MQTT_BROKER];

  #define CONF_DEFAULT_MQTT_TOPIC    "#"
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

  #define CONF_DEFAULT_MQTT_QOS  2
  int mqtt_qos;

  #define CONF_DEFAULT_VERBOSE  0
  int verbose;

  #define CONF_DEFAULT_SEC   0
  #define CONF_DEFAULT_USEC  0
  struct timeval start_time;

  struct mosquitto *mosq;
  FILE *fd;
  sigset_t sigset;

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
  config.mqtt_qos           = CONF_DEFAULT_MQTT_QOS;
  config.verbose            = CONF_DEFAULT_VERBOSE;

  config.start_time.tv_sec  = CONF_DEFAULT_SEC;
  config.start_time.tv_usec = CONF_DEFAULT_USEC;

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
  printf("Logfile is a file where the recieved messages will be saved.\n\n");
  printf("Options: \n");
  printf("-t --topic          MQTT topic where where the measurements will be posted\n");
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
  printf("-q --qos            The maximal quality of service level with which the recorder will revieve messages.\n");
  printf("                    Possible values: 0-2\n");
  printf("                    Default value: %d\n", CONF_DEFAULT_MQTT_QOS);
  printf("-v --verbose        Print alot information to stdout.\n");
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

    // QOS
    } else if( !strcmp(argv[i], "-q") || !strcmp(argv[i], "--qos") ) {
      if( ++i == argc ) {
        fprintf(stderr, "ERROR: Parameter %s given but no qos level specified.\n", argv[i-1]);
	print_usage(*argv);
	exit(1);
      } else {
        config.mqtt_qos = atoi(argv[i]);
	if( 0 > config.mqtt_qos || 2 < config.mqtt_qos ) {
	  fprintf(stderr, "ERROR: Invalid QoS level given: %d", config.mqtt_qos);
	  print_usage(*argv);
	  exit(1);
	}
      }

    // VERBOSE
    } else if( !strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose") ) {
      config.verbose = 1;

    // UNKNOWN PARAM
    } else if( !strcmp(argv[i], "-h") || !strcmp(argv[i], "--help") ) {
      print_usage(*argv);
      exit(0);

    // FILE
    } else {
      strncpy(config.log_file, argv[i], CONF_MAX_LENGTH_LOG_FILE);
    }
  }
}

void log_callback(struct mosquitto *mosq, void *userdata, int level, char const *str) {
  if( config.verbose ) {
    printf("%s\n", str);
  }
}

void connect_callback(struct mosquitto *mosq, void *userdata, int result) {
  if( !result ) {
    if( gettimeofday(&config.start_time, NULL) ) {
      CRIT("Could not get time.");
    }

    mosquitto_subscribe(mosq, NULL, config.mqtt_topic, config.mqtt_qos);
  } else {
    CRIT("Connection to broker faild.");
  }
}

void message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *msg) {
  int i;
  struct timeval time;
  
  if( gettimeofday(&time, NULL) ) {
    CRIT("Could not get time.");
  }
  
  timersub(&time, &config.start_time, &time);

  if( 0 > sigprocmask(SIG_BLOCK, &config.sigset, NULL ) ) {
    CRIT("sigprocmask(SIG_BLOCK)");
  }

  fprintf(config.fd, "msg");
  fprintf(config.fd, " %zd.%06zd", time.tv_sec, time.tv_usec);
  fprintf(config.fd, " %d", msg->qos);
  fprintf(config.fd, " %d", msg->retain);
  fprintf(config.fd, " %d", msg->payloadlen);
  fprintf(config.fd, " %s", msg->topic);
  fprintf(config.fd, "\n");

  if( 0 < msg->payloadlen ) {
    fprintf(config.fd, "%02hx", ((unsigned char *)msg->payload)[0]);
  }

  for( i = 1; i < msg->payloadlen; i++ ) {
    fprintf(config.fd, " %02hx", ((unsigned char *)msg->payload)[i]);
  }

  fprintf(config.fd, "\n");

  if( 0 > sigprocmask(SIG_UNBLOCK, &config.sigset, NULL ) ) {
    CRIT("sigprocmask(SIG_UNBLOCK)");
  }
}

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
  struct timeval time;

  if( config_init() ) {
    CRIT("Faild to initialize config.");
  }
  
  parse_args(argc, argv);

  if( !strlen(config.log_file) ) {
    fprintf(stderr, "ERROR: You have to provide a logfile.\n");
    print_usage(*argv);
    exit(1);
  }

  config.fd = fopen(config.log_file, "w");
  if( NULL == config.fd ) {
    CRIT("Could not open log file.");
  }

  if( gettimeofday(&time, NULL) ) {
    CRIT("Could not get time.");
  }

  fprintf(config.fd,"cnf time: %zd.%06zd\n", time.tv_sec, time.tv_usec);

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

  mosquitto_log_callback_set(config.mosq, log_callback);
  mosquitto_connect_callback_set(config.mosq, connect_callback);
  mosquitto_message_callback_set(config.mosq, message_callback);

  mosquitto_connect(config.mosq, config.mqtt_broker, config.mqtt_port, config.mqtt_keepalive);

  while( !mosquitto_loop_forever(config.mosq, -1, 100) );

  mosquitto_destroy(config.mosq);

  mosquitto_lib_cleanup();

  fclose(config.fd);

  return 0;
}


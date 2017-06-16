/* libcgate - library to send and receve messages from Clipsal's C-Gate server.
 * Copyright (C) 2017, Andrew Tarabaras.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "libcgate.h"

#define DEBUG

#if defined DEBUG
    #define DEBUG_PRINT(fmt, ...) do { printf("%s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0)
#else
    #define DEBUG_PRINT(fmt, ...) /* Don't do anything in release builds */
#endif

static void (*cgate_lighting_event_handler)(uint8_t,
                                            uint8_t,
                                            uint8_t,
                                            uint8_t,
                                            uint8_t);

static void (*cgate_measurement_event_handler)(uint8_t,
                                            uint8_t,
                                            uint8_t,
                                            uint8_t,
                                            int32_t,
                                            int8_t,
                                            cbus_measurement_unit_type);

void cgate_lighting_register_event_handler(void (*f)(uint8_t,
                                                     uint8_t,
                                                     uint8_t,
                                                     uint8_t,
                                                     uint8_t))
{
  cgate_lighting_event_handler = f;
}

void cgate_measurement_register_handler(void (*f)(uint8_t,
                                                  uint8_t,
                                                  uint8_t,
                                                  uint8_t,
                                                  int32_t,
                                                  int8_t,
                                                  cbus_measurement_unit_type))
{
  cgate_measurement_event_handler = f;
}

int8_t *get_cgate_value(int8_t *buf)
{
    int32_t i;
    static int8_t ret[32];
    for(i=0; i<=255;i++){
        if(buf[i]=='='){
            buf +=i;
            break;
        }
    }
    if(*buf != '=')
        return NULL;

    for(i=0; i< 32; i++){
        if(buf[i] == '\r'){
            ret[i] = '\0';
            return ret;
        }
        ret[i] = buf[i+1];
    }
    /* Should not get here */
    return NULL;
}

int32_t cgate_get_ok(int sockfd)
{
    int n;
    int8_t buf[255];
    while(1){
        n = read(sockfd,buf,255);
        buf[7] = '\0';
        n = strncmp((const char*)buf,"200 OK", 6);
        if(n==0)
            return 0;
    }

    DEBUG_PRINT("ERROR sending to bus: %s n=%d\n", buf,n);
    return 1;
}

int32_t cgate_set_group(int sockfd, uint8_t net, uint8_t app, uint8_t group, uint8_t value)
{
    char buf[255];
    if(value == 0)
        sprintf(buf,"OFF %d/%d/%d\r",net,app,group);
    else
        sprintf(buf,"ON %d/%d/%d\r",net,app,group);

    if(write(sockfd,buf,strlen(buf)) < 0)
        return -1;
    if(cgate_get_ok(sockfd))
        return 0;
    DEBUG_PRINT("Failed to set group\n");
    return -1;
}

int32_t cgate_set_ramp(int sockfd, uint8_t net, uint8_t app, uint8_t group, uint8_t value, uint8_t ramprate)
{
    char buf[255];
    sprintf(buf, "ramp %d/%d/%d %d %d\r", net, app, group, value, ramprate);
    if(write(sockfd,buf,strlen(buf)) < 0)
        return -1;
    if(cgate_get_ok(sockfd))
        return 0;
    DEBUG_PRINT("Failed to set group\n");
    return -1;
}

int32_t cgate_send_measurement(int sockfd, uint8_t net, uint8_t app, uint8_t device, uint8_t channel, int32_t value, int8_t exponent, cbus_measurement_unit_type units)
{
    char buf[255];
    sprintf(buf, "measurement data %d/%d/%d/%d %d %d %d\r", net, app, device, channel, value, exponent, units);
    if(write(sockfd,buf,strlen(buf)) < 0)
        return -1;
    if(cgate_get_ok(sockfd))
        return 0;
    DEBUG_PRINT("Failed to set group\n");
    return -1;

}

void cgate_process_lighting_event(uint8_t *buf)
{
    char *ret, *ptr;
    long val;
    u_int8_t value, net, group, application, ramp;
    char project[32];
    int i;

    if(strstr((const char*)buf,"lighting on") != NULL){
        DEBUG_PRINT("On \n");
        value = 255;
        ret = strstr((const char*)buf,"//");
        i = strstr((const char*)ret+2,"/") - (ret+2);
        memcpy(project,ret+2,i);
        project[i] = '\0';
        DEBUG_PRINT("PROJECT = %s\n",project);
        ret += (i+3);
        val = strtol(ret,&ptr,10);
        net = (u_int8_t)val;
        DEBUG_PRINT("NETWORK = %d\n",(int)val);
        ret = ptr+1;
        val = strtol(ret,&ptr,10);
        application = (u_int8_t)val;
        DEBUG_PRINT("APP = %d\n",(int)val);
        ret = ptr+1;
        val = strtol((const char*)ret,&ptr,10);
        group = (u_int8_t)val;
        DEBUG_PRINT("GROUP = %d\n",(int)val);
        ramp = 0;
        if((void (*)(void))cgate_lighting_event_handler)
            cgate_lighting_event_handler(net,application,group,value,ramp);
    }

    if(strstr((const char*)buf,"lighting off") != NULL){
        DEBUG_PRINT("Off \n");
        value = 0;
        ret = strstr((const char*)buf,"//");
        i = strstr((const char*)ret+2,"/") - (ret+2);
        memcpy(project,ret+2,i);
        project[i] = '\0';
        DEBUG_PRINT("PROJECT = %s\n",project);
        ret += (i+3);
        val = strtol(ret,&ptr,10);
        net = (u_int8_t)val;
        DEBUG_PRINT("NETWORK = %d\n",(int)val);
        ret = ptr+1;
        val = strtol(ret,&ptr,10);
        application = (u_int8_t)val;
        DEBUG_PRINT("APP = %d\n",(int)val);
        ret = ptr+1;
        val = strtol((const char*)ret,&ptr,10);
        group = (u_int8_t)val;
        DEBUG_PRINT("GROUP = %d\n",(int)val);
        ramp = 0;
        if((void (*)(void))cgate_lighting_event_handler)
            cgate_lighting_event_handler(net,application,group,value,ramp);
    }

    if(strstr((const char*)buf,"lighting ramp") != NULL){
        DEBUG_PRINT("Ramp \n");
        ret = strstr((const char*)buf,"//");
        i = strstr((const char*)ret+2,"/") - (ret+2);
        memcpy(project,ret+2,i);
        project[i] = '\0';
        DEBUG_PRINT("PROJECT = %s\n",project);
        ret += (i+3);
        val = strtol(ret,&ptr,10);
        net = (u_int8_t)val;
        DEBUG_PRINT("NETWORK = %d\n",(int)val);
        ret = ptr+1;
        val = strtol(ret,&ptr,10);
        application = (u_int8_t)val;
        DEBUG_PRINT("APP = %d\n",(int)val);
        ret = ptr+1;
        val = strtol((const char*)ret,&ptr,10);
        group = (u_int8_t)val;
        DEBUG_PRINT("GROUP = %d\n",(int)val);
        ret = strstr((const char*)ret," ");
        ret++;
        val = strtol((const char*)ret,&ptr,10);
        value = (u_int8_t)val;
        DEBUG_PRINT("RAMP = %d\n",(int)val);
        ret = strstr((const char*)ret," ");
        ret++;
        val = strtol((const char*)ret,&ptr,10);
        ramp = (u_int8_t)val;
        DEBUG_PRINT("RAMP RATE = %d\n",(int)val);
        if((void (*)(void))cgate_lighting_event_handler)
            cgate_lighting_event_handler(net,application,group,value,ramp);
    }
}

void cgate_process_measurement_event(uint8_t *buf)
{
    char *ret, *ptr;
    long val;
    int8_t exponent;
    u_int8_t net, device, channel, application;
    char project[32];
    int32_t i, value;
    cbus_measurement_unit_type type;

    if(strstr((const char*)buf,"measurement data") != NULL){
        DEBUG_PRINT("Measurement Data \n");
        ret = strstr((const char*)buf,"//");
        i = strstr((const char*)ret+2,"/") - (ret+2);
        memcpy(project,ret+2,i);
        project[i] = '\0';
        DEBUG_PRINT("PROJECT = %s\n",project);
        ret += (i+3);
        val = strtol(ret,&ptr,10);
        net = (u_int8_t)val;
        DEBUG_PRINT("NETWORK = %d\n",(int)val);
        ret = ptr+1;
        val = strtol(ret,&ptr,10);
        application = (u_int8_t)val;
        DEBUG_PRINT("APP = %d\n",(int)val);
        ret = ptr+1;
        val = strtol((const char*)ret,&ptr,10);
        device = (u_int8_t)val;
        DEBUG_PRINT("DEVICE = %d\n",(int)val);
        ret = ptr+1;
        val = strtol((const char*)ret,&ptr,10);
        channel = (u_int8_t)val;
        DEBUG_PRINT("CHANNEL = %d\n",(int)val);
        ret = strstr((const char*)ret," ");
        ret++;
        val = strtol((const char*)ret,&ptr,10);
        value = (int32_t)val;
        DEBUG_PRINT("VALUE = %d\n",(int)val);
        ret = strstr((const char*)ret," ");
        ret++;
        val = strtol((const char*)ret,&ptr,10);
        exponent = (int8_t)val;
        DEBUG_PRINT("EXPONENT  = %d\n",(int)val);
        ret = strstr((const char*)ret," ");
        ret++;
        val = strtol((const char*)ret,&ptr,10);
        type = (u_int8_t)val;
        DEBUG_PRINT("TYPE  = %d\n",(int)val);


        if((void (*)(void))cgate_measurement_event_handler)
            cgate_measurement_event_handler(net,application,device,channel,value,exponent,type);
    }
}

void cgate_process_event(uint8_t *buf)
{
    if(strstr((const char*)buf,"lighting") != NULL)
        cgate_process_lighting_event(buf);
    if(strstr((const char*)buf,"measurement") != NULL)
        cgate_process_measurement_event(buf);


}

void cgate_receive_character(uint8_t rx)
{
    static uint8_t buf[255];
    static uint8_t index=0;

    if(rx == '\n')
        return;

    if(rx == '\r')
    {
        buf[index] = '\0';
        DEBUG_PRINT("Event Buffer: %s\n",buf);
        cgate_process_event(buf);
        index=0;
        memset(buf, 0, sizeof(buf));
    }
    else
    {
        buf[index] = rx;
        index++;
    }
}


void *cgate_event(void *eventfd)
{
    int i, bytes_read;
    uint8_t buf[255];

    while(1){
        bytes_read = read((*(int*)eventfd), buf, 255);
        for(i = 0; i < bytes_read; i++)
            cgate_receive_character(buf[i]);
    }
}

int cgate_connect(char* ip, int portno, int8_t *project, uint8_t net)
{
    static int sockfd, eventfd;
    char buf[255];
    struct sockaddr_in serverAddr, serverAddr2;
    pthread_t sig_thr_id;

    /* Close sockets if doing a reconnect */
    if(sockfd)
        close(sockfd);
    if(eventfd)
        close(eventfd);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        DEBUG_PRINT("ERROR opening socket");

    eventfd = socket(AF_INET, SOCK_STREAM, 0);
    if (eventfd < 0) 
        DEBUG_PRINT("ERROR opening socket");

    /*---- Configure settings of the server address struct ----*/
    /* Address family = Internet */
    serverAddr.sin_family = AF_INET;
    serverAddr2.sin_family = AF_INET;
    /* Set port number, using htons function to use proper byte order */
    serverAddr.sin_port = htons(portno);
    serverAddr2.sin_port = htons(portno+2);
    /* Set IP address to localhost */
    serverAddr.sin_addr.s_addr = inet_addr(ip);  /* IP address */
    serverAddr2.sin_addr.s_addr = inet_addr(ip);
    /* Set all bits of the padding field to 0 */
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
    memset(serverAddr2.sin_zero, '\0', sizeof serverAddr2.sin_zero);

    if (connect(sockfd,(struct sockaddr *)&serverAddr,sizeof(serverAddr)) < 0)
        return -1;

    if (connect(eventfd,(struct sockaddr *)&serverAddr2,sizeof(serverAddr2)) < 0)
        return -1;

    if(read(sockfd,buf,255) < 0){
        DEBUG_PRINT("Socket read error\n");
        return -1;
    }

    if(!strncmp((const char*)buf,"201 Service ready",17))
        DEBUG_PRINT("Connected to C-Gate\n");
    else{
        DEBUG_PRINT("Could not connected to C-Gate\n");
        return -1;
    }

    /* Load the project */
    sprintf(buf, "project load %s\r", project);
    if(write(sockfd,buf,strlen(buf)) < 0)
        DEBUG_PRINT("Write to socket error\n");
    if(!cgate_get_ok(sockfd))
        DEBUG_PRINT("Project load=%s\n", project);
    else
        DEBUG_PRINT("Can not load project\n");

    /* Use The project */
    sprintf(buf, "project use %s\r", project);
    if(write(sockfd,buf,strlen(buf)) < 0)
        DEBUG_PRINT("Write to socket error\n");
    if(!cgate_get_ok(sockfd))
        DEBUG_PRINT("Project Use = %s\n", project);
    else
        DEBUG_PRINT("Can not use project\n");

    /* Open the network */
    sprintf(buf, "net open %d\r",net);
    if(write(sockfd,buf,strlen(buf)) < 0)
        DEBUG_PRINT("Write to socket error\n");
    if(!cgate_get_ok(sockfd))
        DEBUG_PRINT("Network load=%d\n", net);
    else
        DEBUG_PRINT("Can not load project\n");

    /* Start the project */
    sprintf(buf, "project start\r");
    if(write(sockfd,buf,strlen(buf)) < 0)
        DEBUG_PRINT("Write to socket error\n");
    if(!cgate_get_ok(sockfd))
        DEBUG_PRINT("Project started=%s\n", project);
    else
        DEBUG_PRINT("Can not start project\n");



    if(pthread_create (&sig_thr_id, NULL, cgate_event, &eventfd))
        return -1;
    return sockfd;

}

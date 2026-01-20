#include <stdio.h>
#include <nuttx/can.h>
#include <netutils/netlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <nuttx/net/netdev.h>
#include "car_can.h"

// Allocate all CAN RX msg
struct car_can_inverter_qd_currents_t  inverter_qd_currents_msg;
struct car_can_inverter_rms_currents_t tnverter_rms_currents_msg;
struct car_can_system_voltages_t system_voltages_msg;
struct car_can_inverter_info_t inverter_info_msg;
struct car_can_inverter_faults_t inverter_faults_msg;
struct car_can_inverter_temp_t inverter_temp_msg;
struct car_can_foc_vars_t foc_vars_msg;


const int tgtest_canfd_on = 1;

void sigterm_tgtest(int signo)
{
	
}

void readCan(struct canfd_frame *frame)
{
	switch (frame->can_id & 0x0FFFFFFF)
	{
	case CAR_CAN_INVERTER_QD_CURRENTS_FRAME_ID:
		car_can_inverter_qd_currents_unpack( &inverter_qd_currents_msg, frame->data, CAR_CAN_INVERTER_QD_CURRENTS_LENGTH);
		break;
	case CAR_CAN_INVERTER_RMS_CURRENTS_FRAME_ID:
	    car_can_inverter_qd_currents_unpack( &inverter_qd_currents_msg, frame->data, CAR_CAN_INVERTER_RMS_CURRENTS_LENGTH);
		break;
	default:
		break;
	}


}

int main(int argc, char *argv[])
{
	fd_set rdfs;
	unsigned char timestamp = 0;
	unsigned char hwtimestamp = 0;
	unsigned char down_causes_exit = 1;
	unsigned char dropmonitor = 0;
	unsigned char extra_msg_info = 0;
	unsigned char silentani = 0;
	unsigned char color = 0;
	unsigned char view = 0;
	unsigned char log = 0;
	unsigned char logfrmt = 0;
	int count = 0;
	int rcvbuf_size = 0;
	int opt;
	int currmax, numfilter;
	int join_filter;
	char *ptr, *nptr;
	struct sockaddr_can addr;
	struct iovec iov;
	struct msghdr msg;
	struct cmsghdr *cmsg;
	struct can_filter *rfilter;
	can_err_mask_t err_mask;
	struct canfd_frame frame;
	int nbytes, i, maxdlen;
	struct ifreq ifr;
	char ctrlmsg[CMSG_SPACE(sizeof(struct timeval) + 3 * sizeof(struct timespec) + sizeof(uint32_t))];
	char candev[5] ="can0";
	struct timeval timeout, timeout_config = { 0, 0 }, *timeout_current = NULL;
	signal(SIGINT, sigterm_tgtest);


	printf("Hello, tgtest!!\n");
	netlib_ifup("can0");

	/* Create CAN socket */
	int sock = socket(AF_CAN, SOCK_RAW, CAN_RAW);

	/* Bind to CAN interface */
	addr.can_family = AF_CAN;
	memset(&ifr.ifr_name, 0, sizeof(ifr.ifr_name));
	ptr = candev;
	strncpy(ifr.ifr_name, ptr, 5);
	if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
				perror("SIOCGIFINDEX");
				exit(1);
			}
	addr.can_ifindex = ifr.ifr_ifindex;
	//addr.can_ifindex = if_nametoindex("can0");
	setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &tgtest_canfd_on, sizeof(tgtest_canfd_on));
	//setsockopt(s[i], SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &canfd_on, sizeof(canfd_on));
	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
			perror("bind");
			return 1;
		}


	/* Fill data */
	// send(sock, &frame, sizeof(frame), 0);

	/* these settings are static and can be held out of the hot path */
	iov.iov_base = &frame;
	msg.msg_name = &addr;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = &ctrlmsg;

	while (1)
	{
		FD_ZERO(&rdfs);
		FD_SET(sock, &rdfs);

		if ((select(sock+1, &rdfs, NULL, NULL, timeout_current)) <= 0) {
			//perror("select");
			//running = 0;
			continue;
		}

		/* check all CAN RAW sockets */

		if (FD_ISSET(sock, &rdfs))
		{

			int idx;
			/* these settings may be modified by recvmsg() */
			iov.iov_len = sizeof(frame);
			msg.msg_namelen = sizeof(addr);
			msg.msg_controllen = sizeof(ctrlmsg);
			msg.msg_flags = 0;

			nbytes = recvmsg(sock, &msg, 0);
			readCan(&frame);
			/* code */
			/* Receive CAN frame */
			// recv(sock, &frame, sizeof(frame), 0);
		}
		//nxsched_msleep(2);
	}
}
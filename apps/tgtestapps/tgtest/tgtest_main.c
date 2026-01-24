#include <stdio.h>
#include <nuttx/can.h>
#include <netutils/netlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <nuttx/net/netdev.h>
#include "car_can.h"
#include <spawn.h>
#include <nuttx/sched.h>

// Allocate all CAN RX msg
struct car_can_inverter_qd_currents_t inverter_qd_currents_msg;
struct car_can_inverter_rms_currents_t tnverter_rms_currents_msg;
struct car_can_system_voltages_t system_voltages_msg;
struct car_can_inverter_info_t inverter_info_msg;
struct car_can_inverter_faults_t inverter_faults_msg;
struct car_can_inverter_temp_t inverter_temp_msg;
struct car_can_foc_vars_t foc_vars_msg;

const int tgtest_canfd_on = 1;

//pthread_startroutine_t canSend();

void sigterm_tgtest(int signo)
{
}

void readCan(struct canfd_frame *frame)
{
	switch (frame->can_id & CAN_EFF_MASK)
	{
	case CAR_CAN_INVERTER_QD_CURRENTS_FRAME_ID:
		car_can_inverter_qd_currents_unpack(&inverter_qd_currents_msg, frame->data, CAR_CAN_INVERTER_QD_CURRENTS_LENGTH);
		break;
	case CAR_CAN_INVERTER_RMS_CURRENTS_FRAME_ID:
		car_can_inverter_qd_currents_unpack(&inverter_qd_currents_msg, frame->data, CAR_CAN_INVERTER_RMS_CURRENTS_LENGTH);
		break;
	case CAR_CAN_INVERTER_FAULTS_FRAME_ID:
		car_can_inverter_faults_unpack(&inverter_faults_msg, frame->data, CAR_CAN_INVERTER_FAULTS_LENGTH);
		break;
	default:
		break;
	}
}
int sockSend;
static void *canSend(void *arg)
{
	
	struct ifreq ifrSend;
	struct sockaddr_can addrSend;
	struct can_frame frameSend;
	/* open socket */

	// if ((sockSend = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
	// {
    // 	perror("socket");
	// 	return 1;
	// }
	// ifrSend.ifr_ifindex = if_nametoindex("can0");
	// if (!ifrSend.ifr_ifindex)
	// {
	// 	perror("if_nametoindex");
	// 	return 1;
	// }

	// memset(&addrSend, 0, sizeof(addrSend));
	// addrSend.can_family = AF_CAN;
	// addrSend.can_ifindex = ifrSend.ifr_ifindex;

	/* disable default receive filter on this RAW socket
	 * This is obsolete as we do not read from the socket at all, but for
	 * this reason we can remove the receive list in the Kernel to save a
	 * little (really a very little!) CPU usage.
	 */

	//setsockopt(sockSend, SOL_CAN_RAW, CAN_RAW_FILTER, NULL, 0);

	// if (bind(sockSend, (struct sockaddr *)&addrSend, sizeof(addrSend)) < 0)
	// {
	// 	perror("bind");
	// 	return 1;
	// }

	while (1)
	{
		/* send frame */

		struct car_can_driver_commands_t driver_commands_msg;
		driver_commands_msg.dc_link_active_demand = CAR_CAN_DRIVER_COMMANDS_DC_LINK_ACTIVE_DEMAND_ACTIVE_DEMAND_CHOICE;
		driver_commands_msg.demanded_drive_direction = CAR_CAN_DRIVER_COMMANDS_DEMANDED_DRIVE_DIRECTION_FORWARD_CHOICE;
		driver_commands_msg.demanded_drive_state = CAR_CAN_DRIVER_COMMANDS_DEMANDED_DRIVE_STATE_ENABLE_CHOICE;


		car_can_driver_commands_pack(frameSend.data, &driver_commands_msg, CAR_CAN_DRIVER_COMMANDS_LENGTH);
		
		frameSend.can_id = (CAR_CAN_DRIVER_COMMANDS_FRAME_ID & CAN_EFF_MASK)| CAN_EFF_FLAG;
		frameSend.can_dlc = CAR_CAN_DRIVER_COMMANDS_LENGTH;

		if (write(sockSend, &frameSend, CAN_MTU ) != CAN_MTU)
		{
			perror("write");
			//return 1;
		}
		nxsched_msleep(1000);
	}
	return NULL;
}



int main(int argc, char *argv[])
{
	fd_set rdfs;
	char *ptr;
	struct sockaddr_can addr;
	struct iovec iov;
	struct msghdr msg;
	int nbytes;
	struct canfd_frame frame;
	struct ifreq ifr;
	char ctrlmsg[CMSG_SPACE(sizeof(struct timeval) + 3 * sizeof(struct timespec) + sizeof(uint32_t))];
	char candev[5] = "can0";
	struct timeval *timeout_current = NULL;
	signal(SIGINT, sigterm_tgtest);

	printf("Hello, tgtest!!\n");
	netlib_ifup("can0");

	/* Create CAN socket */
	int sock = socket(AF_CAN, SOCK_RAW, CAN_RAW);
	sockSend = sock;

	pthread_t thread;
	pthread_attr_t attr;
	struct sched_param param;

	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 2048);
	param.sched_priority = SCHED_PRIORITY_DEFAULT;
	pthread_attr_setschedparam(&attr, &param);

	int ret = pthread_create(&thread, &attr, canSend, NULL);
	if (ret != 0)
	{
		printf("Error: pthread_create failed: %d\n", ret);
		return 1;
	}

	//pthread_join(thread, NULL);

	

	/* Bind to CAN interface */
	addr.can_family = AF_CAN;
	memset(&ifr.ifr_name, 0, sizeof(ifr.ifr_name));
	ptr = candev;
	strncpy(ifr.ifr_name, ptr, 5);
	if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0)
	{
		perror("SIOCGIFINDEX");
		exit(1);
	}
	addr.can_ifindex = ifr.ifr_ifindex;
	// addr.can_ifindex = if_nametoindex("can0");
	setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &tgtest_canfd_on, sizeof(tgtest_canfd_on));
	// setsockopt(s[i], SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &canfd_on, sizeof(canfd_on));
	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
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

		if ((select(sock + 1, &rdfs, NULL, NULL, timeout_current)) <= 0)
		{
			// perror("select");
			// running = 0;
			continue;
		}

		/* check all CAN RAW sockets */

		if (FD_ISSET(sock, &rdfs))
		{

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
		// nxsched_msleep(2);
	}
}
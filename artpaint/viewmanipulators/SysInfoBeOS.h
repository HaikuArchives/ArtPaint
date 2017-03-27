/*
* Recreation of the original BeOS system_info struct
* to provide legacy apps with cpu_speed, etc.
*
* (The original enums for cpu_type and platform_type
* are here just ints)
*/

#ifdef __cplusplus
extern "C" {
#endif


#define		B_MAX_CPU_COUNT		8


typedef struct {
	bigtime_t		active_time;		/* # usec doing useful work since boot */
} BeOS_cpu_info;


typedef int32 machine_id[2];		/* unique machine ID */

typedef struct {
	machine_id	   id;							/* unique machine ID */
	bigtime_t	   boot_time;					/* time of boot (# usec since 1/1/70) */

	int32		   cpu_count;					/* # of cpus */
	int32          cpu_type;					/* type of cpu */
	int32		   cpu_revision;				/* revision # of cpu */
	BeOS_cpu_info  cpu_infos[B_MAX_CPU_COUNT];	/* info about individual cpus */
	int64          cpu_clock_speed;	 			/* processor clock speed (Hz) */
	int64          bus_clock_speed;				/* bus clock speed (Hz) */
	int32          platform_type;          /* type of machine we're on */

	int32		  max_pages;					/* total # physical pages */
	int32		  used_pages;					/* # physical pages in use */
	int32		  page_faults;					/* # of page faults */
	int32		  max_sems;						/* maximum # semaphores */
	int32		  used_sems;					/* # semaphores in use */
	int32		  max_ports;					/* maximum # ports */
	int32		  used_ports;					/* # ports in use */
	int32		  max_threads;					/* maximum # threads */
	int32		  used_threads;					/* # threads in use */
	int32		  max_teams;					/* maximum # teams */
	int32		  used_teams;					/* # teams in use */

	char		  kernel_name [B_FILE_NAME_LENGTH];		/* name of kernel */
	char          kernel_build_date[B_OS_NAME_LENGTH];	/* date kernel built */
	char          kernel_build_time[B_OS_NAME_LENGTH];	/* time kernel built */
	int64         kernel_version;             	/* version of this kernel */

	bigtime_t	  _busy_wait_time;				/* reserved for Be */
	int32         pad[4];   	               	/* just in case... */
} BeOS_system_info;


extern  status_t _get_system_info (BeOS_system_info *returned_info, size_t size);
#define get_BeOS_system_info(info)  _get_system_info((info), sizeof(*(info)))

#ifdef __cplusplus
}
#endif

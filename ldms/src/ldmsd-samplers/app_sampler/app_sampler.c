/* -*- c-basic-offset: 8 -*-
 * Copyright (c) 2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS). Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 * Copyright (c) 2020 Open Grid Computing, Inc. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the BSD-type
 * license below:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *      Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *      Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 *      Neither the name of Sandia nor the names of any contributors may
 *      be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 *      Neither the name of Open Grid Computing nor the names of any
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *      Modified source versions must be plainly marked as such, and
 *      must not be misrepresented as being the original software.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file app_sampler.c
 */

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dirent.h>
#include <ctype.h>

#include "ldmsd.h"
#include "ldmsd_sampler.h"
#include "ldmsd_stream.h"

#define DEFAULT_STREAM "slurm"

#define INST(x) ((ldmsd_plugin_inst_t)(x))
#define INST_LOG(inst, lvl, fmt, ...) \
		ldmsd_log((lvl), "%s: " fmt, INST(inst)->inst_name, \
								##__VA_ARGS__)

#ifndef ARRAY_LEN
#define ARRAY_LEN(a) (sizeof((a))/sizeof((a)[0]))
#endif

typedef enum app_sampler_metric app_sampler_metric_e;
enum app_sampler_metric {
	APP_ALL = 0,

	/* /proc/[pid]/cmdline */
	APP_CMDLINE_LEN, _APP_CMDLINE_FIRST = APP_CMDLINE_LEN,
			 _APP_FIRST = _APP_CMDLINE_FIRST,
	APP_CMDLINE, _APP_CMDLINE_LAST = APP_CMDLINE,

	/* number of open files */
	APP_N_OPEN_FILES,

	/* io */
	APP_IO_READ_B, _APP_IO_FIRST = APP_IO_READ_B,
	APP_IO_WRITE_B,
	APP_IO_N_READ,
	APP_IO_N_WRITE,
	APP_IO_READ_DEV_B,
	APP_IO_WRITE_DEV_B,
	APP_IO_WRITE_CANCELLED_B, _APP_IO_LAST = APP_IO_WRITE_CANCELLED_B,

	APP_OOM_SCORE,

	APP_OOM_SCORE_ADJ,

	APP_ROOT,

	/* /proc/[pid]/stat */
	APP_STAT_PID, _APP_STAT_FIRST = APP_STAT_PID,
	APP_STAT_COMM,
	APP_STAT_STATE,
	APP_STAT_PPID,
	APP_STAT_PGRP,
	APP_STAT_SESSION,
	APP_STAT_TTY_NR,
	APP_STAT_TPGID,
	APP_STAT_FLAGS,
	APP_STAT_MINFLT,
	APP_STAT_CMINFLT,
	APP_STAT_MAJFLT,
	APP_STAT_CMAJFLT,
	APP_STAT_UTIME,
	APP_STAT_STIME,
	APP_STAT_CUTIME,
	APP_STAT_CSTIME,
	APP_STAT_PRIORITY,
	APP_STAT_NICE,
	APP_STAT_NUM_THREADS,
	APP_STAT_ITREALVALUE,
	APP_STAT_STARTTIME,
	APP_STAT_VSIZE,
	APP_STAT_RSS,
	APP_STAT_RSSLIM,
	APP_STAT_STARTCODE,
	APP_STAT_ENDCODE,
	APP_STAT_STARTSTACK,
	APP_STAT_KSTKESP,
	APP_STAT_KSTKEIP,
	APP_STAT_SIGNAL,
	APP_STAT_BLOCKED,
	APP_STAT_SIGIGNORE,
	APP_STAT_SIGCATCH,
	APP_STAT_WCHAN,
	APP_STAT_NSWAP,
	APP_STAT_CNSWAP,
	APP_STAT_EXIT_SIGNAL,
	APP_STAT_PROCESSOR,
	APP_STAT_RT_PRIORITY,
	APP_STAT_POLICY,
	APP_STAT_DELAYACCT_BLKIO_TICKS,
	APP_STAT_GUEST_TIME,
	APP_STAT_CGUEST_TIME,
	APP_STAT_START_DATA,
	APP_STAT_END_DATA,
	APP_STAT_START_BRK,
	APP_STAT_ARG_START,
	APP_STAT_ARG_END,
	APP_STAT_ENV_START,
	APP_STAT_ENV_END,
	APP_STAT_EXIT_CODE, _APP_STAT_LAST = APP_STAT_EXIT_CODE,

	APP_STATUS_NAME, _APP_STATUS_FIRST = APP_STATUS_NAME,
	APP_STATUS_UMASK,
	APP_STATUS_STATE,
	APP_STATUS_TGID,
	APP_STATUS_NGID,
	APP_STATUS_PID,
	APP_STATUS_PPID,
	APP_STATUS_TRACERPID,
	APP_STATUS_UID,
	APP_STATUS_GID,
	APP_STATUS_FDSIZE,
	APP_STATUS_GROUPS,
	APP_STATUS_NSTGID,
	APP_STATUS_NSPID,
	APP_STATUS_NSPGID,
	APP_STATUS_NSSID,
	APP_STATUS_VMPEAK,
	APP_STATUS_VMSIZE,
	APP_STATUS_VMLCK,
	APP_STATUS_VMPIN,
	APP_STATUS_VMHWM,
	APP_STATUS_VMRSS,
	APP_STATUS_RSSANON,
	APP_STATUS_RSSFILE,
	APP_STATUS_RSSSHMEM,
	APP_STATUS_VMDATA,
	APP_STATUS_VMSTK,
	APP_STATUS_VMEXE,
	APP_STATUS_VMLIB,
	APP_STATUS_VMPTE,
	APP_STATUS_VMPMD,
	APP_STATUS_VMSWAP,
	APP_STATUS_HUGETLBPAGES,
	APP_STATUS_COREDUMPING,
	APP_STATUS_THREADS,
	APP_STATUS_SIG_QUEUED,
	APP_STATUS_SIG_LIMIT,
	APP_STATUS_SIGPND,
	APP_STATUS_SHDPND,
	APP_STATUS_SIGBLK,
	APP_STATUS_SIGIGN,
	APP_STATUS_SIGCGT,
	APP_STATUS_CAPINH,
	APP_STATUS_CAPPRM,
	APP_STATUS_CAPEFF,
	APP_STATUS_CAPBND,
	APP_STATUS_CAPAMB,
	APP_STATUS_NONEWPRIVS,
	APP_STATUS_SECCOMP,
	APP_STATUS_SPECULATION_STORE_BYPASS,
	APP_STATUS_CPUS_ALLOWED,
	APP_STATUS_CPUS_ALLOWED_LIST,
	APP_STATUS_MEMS_ALLOWED,
	APP_STATUS_MEMS_ALLOWED_LIST,
	APP_STATUS_VOLUNTARY_CTXT_SWITCHES,
	APP_STATUS_NONVOLUNTARY_CTXT_SWITCHES,
	_APP_STATUS_LAST = APP_STATUS_NONVOLUNTARY_CTXT_SWITCHES,

	APP_SYSCALL,

	APP_TIMERSLACK_NS,

	APP_WCHAN, /* string */

	_APP_LAST = APP_WCHAN,
};

typedef struct app_sampler_metric_info_s *app_sampler_metric_info_t;
struct app_sampler_metric_info_s {
	app_sampler_metric_e code;
	const char *name;
	const char *unit;
	enum ldms_value_type mtype;
	int array_len; /* in case mtype is _ARRAY */
	int is_meta;
};

#define CMDLINE_SZ 4096
#define MOUNTINFO_SZ 8192
#define ROOT_SZ 4096
#define STAT_COMM_SZ 4096
#define WCHAN_SZ 128 /* NOTE: KSYM_NAME_LEN = 128 */
#define SPECULATION_SZ 64
#define GROUPS_SZ 16
#define NS_SZ 16
#define CPUS_ALLOWED_SZ 4 /* 4 x 64 = 256 bits max */
#define MEMS_ALLOWED_SZ 128 /* 128 x 64 = 8192 bits max */
#define CPUS_ALLOWED_LIST_SZ 128 /* length of list string */
#define MEMS_ALLOWED_LIST_SZ 128 /* length of list string */

struct app_sampler_metric_info_s metric_info[] = {
	[APP_ALL] = { APP_ALL, "ALL", "", 0, 0, 0 },
	/* /proc/[pid]/cmdline */
	[APP_CMDLINE_LEN] = { APP_CMDLINE_LEN, "cmdline_len", "" , LDMS_V_U16, 0, 1 },
	[APP_CMDLINE] = { APP_CMDLINE, "cmdline", "", LDMS_V_CHAR_ARRAY, CMDLINE_SZ, 1 },

	/* number of open files */
	[APP_N_OPEN_FILES] = { APP_N_OPEN_FILES, "n_open_files", "", LDMS_V_U64, 0, 0 },

	/* io */
	[APP_IO_READ_B] = { APP_IO_READ_B, "io_read_b", "B", LDMS_V_U64, 0, 0 },
	[APP_IO_WRITE_B] = { APP_IO_WRITE_B, "io_write_b", "B", LDMS_V_U64, 0, 0 },
	[APP_IO_N_READ] = { APP_IO_N_READ, "io_n_read", "", LDMS_V_U64, 0, 0 },
	[APP_IO_N_WRITE] = { APP_IO_N_WRITE, "io_n_write", "", LDMS_V_U64, 0, 0 },
	[APP_IO_READ_DEV_B] = { APP_IO_READ_DEV_B, "io_read_dev_b", "B", LDMS_V_U64, 0, 0 },
	[APP_IO_WRITE_DEV_B] = { APP_IO_WRITE_DEV_B, "io_write_dev_b", "B", LDMS_V_U64, 0, 0 },
	[APP_IO_WRITE_CANCELLED_B] = { APP_IO_WRITE_CANCELLED_B, "io_write_cancelled_b", "B", LDMS_V_U64, 0, 0 },

	[APP_OOM_SCORE] = { APP_OOM_SCORE, "oom_score", "", LDMS_V_U64, 0, 0 },

	[APP_OOM_SCORE_ADJ] = { APP_OOM_SCORE_ADJ, "oom_score_adj", "", LDMS_V_U64, 0, 0 },

	[APP_ROOT] = { APP_ROOT, "root", "", LDMS_V_CHAR_ARRAY, ROOT_SZ, 1 },

	/* /proc/[pid]/stat */
	[APP_STAT_PID] = { APP_STAT_PID, "stat_pid", "", LDMS_V_U64, 0, 0 },
	[APP_STAT_COMM] = { APP_STAT_COMM, "stat_comm", "", LDMS_V_CHAR_ARRAY, STAT_COMM_SZ, 1 },
	[APP_STAT_STATE] = { APP_STAT_STATE, "stat_state", "", LDMS_V_CHAR, 0, 0 },
	[APP_STAT_PPID] = { APP_STAT_PPID, "stat_ppid", "", LDMS_V_U64, 0, 0 },
	[APP_STAT_PGRP] = { APP_STAT_PGRP, "stat_pgrp", "", LDMS_V_U64, 0, 0 },
	[APP_STAT_SESSION] = { APP_STAT_SESSION, "stat_session", "", LDMS_V_U64, 0, 0 },
	[APP_STAT_TTY_NR] = { APP_STAT_TTY_NR, "stat_tty_nr", "", LDMS_V_U64, 0, 0 },
	[APP_STAT_TPGID] = { APP_STAT_TPGID, "stat_tpgid", "", LDMS_V_U64, 0, 0 },
	[APP_STAT_FLAGS] = { APP_STAT_FLAGS, "stat_flags", "", LDMS_V_U64, 0, 0 },
	[APP_STAT_MINFLT] = { APP_STAT_MINFLT, "stat_minflt", "", LDMS_V_U64, 0, 0 },
	[APP_STAT_CMINFLT] = { APP_STAT_CMINFLT, "stat_cminflt", "", LDMS_V_U64, 0, 0 },
	[APP_STAT_MAJFLT] = { APP_STAT_MAJFLT, "stat_majflt", "", LDMS_V_U64, 0, 0 },
	[APP_STAT_CMAJFLT] = { APP_STAT_CMAJFLT, "stat_cmajflt", "", LDMS_V_U64, 0, 0 },
	[APP_STAT_UTIME] = { APP_STAT_UTIME, "stat_utime", "ticks", LDMS_V_U64, 0, 0 },
	[APP_STAT_STIME] = { APP_STAT_STIME, "stat_stime", "ticks", LDMS_V_U64, 0, 0 },
	[APP_STAT_CUTIME] = { APP_STAT_CUTIME, "stat_cutime", "ticks", LDMS_V_U64, 0, 0 },
	[APP_STAT_CSTIME] = { APP_STAT_CSTIME, "stat_cstime", "ticks", LDMS_V_U64, 0, 0 },
	[APP_STAT_PRIORITY] = { APP_STAT_PRIORITY, "stat_priority", "", LDMS_V_U64, 0, 0 },
	[APP_STAT_NICE] = { APP_STAT_NICE, "stat_nice", "", LDMS_V_U64, 0, 0 },
	[APP_STAT_NUM_THREADS] =
		{ APP_STAT_NUM_THREADS, "stat_num_threads", "", LDMS_V_U64, 0, 0 },
	[APP_STAT_ITREALVALUE] =
		{ APP_STAT_ITREALVALUE, "stat_itrealvalue", "ticks", LDMS_V_U64, 0, 0 },
	[APP_STAT_STARTTIME] =
		{ APP_STAT_STARTTIME, "stat_starttime", "ticks", LDMS_V_U64, 0, 0 },
	[APP_STAT_VSIZE] = { APP_STAT_VSIZE, "stat_vsize", "B", LDMS_V_U64, 0, 0 },
	[APP_STAT_RSS] = { APP_STAT_RSS, "stat_rss", "PG", LDMS_V_U64, 0, 0 },
	[APP_STAT_RSSLIM] = { APP_STAT_RSSLIM, "stat_rsslim", "B", LDMS_V_U64, 0, 0 },
	[APP_STAT_STARTCODE] = { APP_STAT_STARTCODE, "stat_startcode", "PTR", LDMS_V_U64, 0, 0 },
	[APP_STAT_ENDCODE] = { APP_STAT_ENDCODE, "stat_endcode", "PTR", LDMS_V_U64, 0, 0 },
	[APP_STAT_STARTSTACK] =
		{ APP_STAT_STARTSTACK, "stat_startstack", "PTR", LDMS_V_U64, 0, 0 },
	[APP_STAT_KSTKESP] = { APP_STAT_KSTKESP, "stat_kstkesp", "PTR", LDMS_V_U64, 0, 0 },
	[APP_STAT_KSTKEIP] = { APP_STAT_KSTKEIP, "stat_kstkeip", "PTR", LDMS_V_U64, 0, 0 },
	[APP_STAT_SIGNAL] = { APP_STAT_SIGNAL, "stat_signal", "", LDMS_V_U64, 0, 0 },
	[APP_STAT_BLOCKED] = { APP_STAT_BLOCKED, "stat_blocked", "", LDMS_V_U64, 0, 0 },
	[APP_STAT_SIGIGNORE] = { APP_STAT_SIGIGNORE, "stat_sigignore", "", LDMS_V_U64, 0, 0 },
	[APP_STAT_SIGCATCH] = { APP_STAT_SIGCATCH, "stat_sigcatch", "", LDMS_V_U64, 0, 0 },
	[APP_STAT_WCHAN] = { APP_STAT_WCHAN, "stat_wchan", "PTR", LDMS_V_U64, 0, 0 },
	[APP_STAT_NSWAP] = { APP_STAT_NSWAP, "stat_nswap", "PG", LDMS_V_U64, 0, 0 },
	[APP_STAT_CNSWAP] = { APP_STAT_CNSWAP, "stat_cnswap", "PG", LDMS_V_U64, 0, 0 },
	[APP_STAT_EXIT_SIGNAL] = { APP_STAT_EXIT_SIGNAL, "stat_exit_signal", "", LDMS_V_U64, 0, 0 },
	[APP_STAT_PROCESSOR] = { APP_STAT_PROCESSOR, "stat_processor", "", LDMS_V_U64, 0, 0 },
	[APP_STAT_RT_PRIORITY] = { APP_STAT_RT_PRIORITY, "stat_rt_priority", "", LDMS_V_U64, 0, 0 },
	[APP_STAT_POLICY] = { APP_STAT_POLICY, "stat_policy", "", LDMS_V_U64, 0, 0 },
	[APP_STAT_DELAYACCT_BLKIO_TICKS] = { APP_STAT_DELAYACCT_BLKIO_TICKS, "stat_delayacct_blkio_ticks", "ticks", LDMS_V_U64, 0, 0 },
	[APP_STAT_GUEST_TIME] = { APP_STAT_GUEST_TIME, "stat_guest_time", "ticks", LDMS_V_U64, 0, 0 },
	[APP_STAT_CGUEST_TIME] = { APP_STAT_CGUEST_TIME, "stat_cguest_time", "ticks", LDMS_V_U64, 0, 0 },
	[APP_STAT_START_DATA] = { APP_STAT_START_DATA, "stat_start_data", "PTR", LDMS_V_U64, 0, 0 },
	[APP_STAT_END_DATA] = { APP_STAT_END_DATA, "stat_end_data", "PTR", LDMS_V_U64, 0, 0 },
	[APP_STAT_START_BRK] = { APP_STAT_START_BRK, "stat_start_brk", "PTR", LDMS_V_U64, 0, 0 },
	[APP_STAT_ARG_START] = { APP_STAT_ARG_START, "stat_arg_start", "PTR", LDMS_V_U64, 0, 0 },
	[APP_STAT_ARG_END] = { APP_STAT_ARG_END, "stat_arg_end", "PTR", LDMS_V_U64, 0, 0 },
	[APP_STAT_ENV_START] = { APP_STAT_ENV_START, "stat_env_start", "PTR", LDMS_V_U64, 0, 0 },
	[APP_STAT_ENV_END] = { APP_STAT_ENV_END, "stat_env_end", "PTR", LDMS_V_U64, 0, 0 },
	[APP_STAT_EXIT_CODE] = { APP_STAT_EXIT_CODE, "stat_exit_code", "", LDMS_V_U64, 0, 0 },

	/* /proc/[pid]/status */
	[APP_STATUS_NAME] = { APP_STATUS_NAME, "status_name", "", LDMS_V_CHAR_ARRAY, STAT_COMM_SZ, 1 },
	[APP_STATUS_UMASK] = { APP_STATUS_UMASK, "status_umask", "", LDMS_V_U32, 0, 0 },
	[APP_STATUS_STATE] = { APP_STATUS_STATE, "status_state", "", LDMS_V_CHAR, 0, 0 },
	[APP_STATUS_TGID] = { APP_STATUS_TGID, "status_tgid", "", LDMS_V_U64, 0, 0 },
	[APP_STATUS_NGID] = { APP_STATUS_NGID, "status_ngid", "", LDMS_V_U64, 0, 0 },
	[APP_STATUS_PID] = { APP_STATUS_PID, "status_pid", "" , LDMS_V_U64, 0, 0},
	[APP_STATUS_PPID] = { APP_STATUS_PPID, "status_ppid", "" , LDMS_V_U64, 0, 0},
	[APP_STATUS_TRACERPID] =
		{ APP_STATUS_TRACERPID, "status_tracerpid", "" , LDMS_V_U64, 0, 0},
	[APP_STATUS_UID] = { APP_STATUS_UID, "status_uid", "", LDMS_V_U64_ARRAY, 4, 0 },
	[APP_STATUS_GID] = { APP_STATUS_GID, "status_gid", "" , LDMS_V_U64_ARRAY, 4, 0},
	[APP_STATUS_FDSIZE] = { APP_STATUS_FDSIZE, "status_fdsize", "" , LDMS_V_U64, 0, 0},
	[APP_STATUS_GROUPS] = { APP_STATUS_GROUPS, "status_groups", "", LDMS_V_U64_ARRAY, GROUPS_SZ, 0 },
	[APP_STATUS_NSTGID] = { APP_STATUS_NSTGID, "status_nstgid", "", LDMS_V_U64_ARRAY, NS_SZ, 0 },
	[APP_STATUS_NSPID] = { APP_STATUS_NSPID, "status_nspid", "", LDMS_V_U64_ARRAY, NS_SZ, 0 },
	[APP_STATUS_NSPGID] = { APP_STATUS_NSPGID, "status_nspgid", "", LDMS_V_U64_ARRAY, NS_SZ, 0 },
	[APP_STATUS_NSSID] = { APP_STATUS_NSSID, "status_nssid", "", LDMS_V_U64_ARRAY, NS_SZ, 0 },
	[APP_STATUS_VMPEAK] = { APP_STATUS_VMPEAK, "status_vmpeak", "kB", LDMS_V_U64, 0, 0 },
	[APP_STATUS_VMSIZE] = { APP_STATUS_VMSIZE, "status_vmsize", "kB", LDMS_V_U64, 0, 0 },
	[APP_STATUS_VMLCK] = { APP_STATUS_VMLCK, "status_vmlck", "kB", LDMS_V_U64, 0, 0 },
	[APP_STATUS_VMPIN] = { APP_STATUS_VMPIN, "status_vmpin", "kB", LDMS_V_U64, 0, 0 },
	[APP_STATUS_VMHWM] = { APP_STATUS_VMHWM, "status_vmhwm", "kB", LDMS_V_U64, 0, 0 },
	[APP_STATUS_VMRSS] = { APP_STATUS_VMRSS, "status_vmrss", "kB", LDMS_V_U64, 0, 0 },
	[APP_STATUS_RSSANON] = { APP_STATUS_RSSANON, "status_rssanon", "kB", LDMS_V_U64, 0, 0 },
	[APP_STATUS_RSSFILE] = { APP_STATUS_RSSFILE, "status_rssfile", "kB", LDMS_V_U64, 0, 0 },
	[APP_STATUS_RSSSHMEM] = { APP_STATUS_RSSSHMEM, "status_rssshmem", "kB", LDMS_V_U64, 0, 0 },
	[APP_STATUS_VMDATA] = { APP_STATUS_VMDATA, "status_vmdata", "kB", LDMS_V_U64, 0, 0 },
	[APP_STATUS_VMSTK] = { APP_STATUS_VMSTK, "status_vmstk", "kB", LDMS_V_U64, 0, 0 },
	[APP_STATUS_VMEXE] = { APP_STATUS_VMEXE, "status_vmexe", "kB", LDMS_V_U64, 0, 0 },
	[APP_STATUS_VMLIB] = { APP_STATUS_VMLIB, "status_vmlib", "kB", LDMS_V_U64, 0, 0 },
	[APP_STATUS_VMPTE] = { APP_STATUS_VMPTE, "status_vmpte", "kB", LDMS_V_U64, 0, 0 },
	[APP_STATUS_VMPMD] = { APP_STATUS_VMPMD, "status_vmpmd", "kB", LDMS_V_U64, 0, 0 },
	[APP_STATUS_VMSWAP] = { APP_STATUS_VMSWAP, "status_vmswap", "kB", LDMS_V_U64, 0, 0 },
	[APP_STATUS_HUGETLBPAGES] = { APP_STATUS_HUGETLBPAGES, "status_hugetlbpages", "kB", LDMS_V_U64, 0, 0 },
	[APP_STATUS_COREDUMPING] = { APP_STATUS_COREDUMPING, "status_coredumping", "bool", LDMS_V_U8, 0, 0 },
	[APP_STATUS_THREADS] = { APP_STATUS_THREADS, "status_threads", "", LDMS_V_U64, 0, 0 },
	[APP_STATUS_SIG_QUEUED] = { APP_STATUS_SIG_QUEUED, "status_sig_queued", "", LDMS_V_U64, 0, 0 },
	[APP_STATUS_SIG_LIMIT] = { APP_STATUS_SIG_LIMIT, "status_sig_limit", "", LDMS_V_U64, 0, 0},
	[APP_STATUS_SIGPND] = { APP_STATUS_SIGPND, "status_sigpnd", "", LDMS_V_U64, 0, 0 },
	[APP_STATUS_SHDPND] = { APP_STATUS_SHDPND, "status_shdpnd", "", LDMS_V_U64, 0, 0 },
	[APP_STATUS_SIGBLK] = { APP_STATUS_SIGBLK, "status_sigblk", "", LDMS_V_U64, 0, 0 },
	[APP_STATUS_SIGIGN] = { APP_STATUS_SIGIGN, "status_sigign", "", LDMS_V_U64, 0, 0 },
	[APP_STATUS_SIGCGT] = { APP_STATUS_SIGCGT, "status_sigcgt", "", LDMS_V_U64, 0, 0 },
	[APP_STATUS_CAPINH] = { APP_STATUS_CAPINH, "status_capinh", "", LDMS_V_U64, 0, 0 },
	[APP_STATUS_CAPPRM] = { APP_STATUS_CAPPRM, "status_capprm", "", LDMS_V_U64, 0, 0 },
	[APP_STATUS_CAPEFF] = { APP_STATUS_CAPEFF, "status_capeff", "", LDMS_V_U64, 0, 0 },
	[APP_STATUS_CAPBND] = { APP_STATUS_CAPBND, "status_capbnd", "", LDMS_V_U64, 0, 0 },
	[APP_STATUS_CAPAMB] = { APP_STATUS_CAPAMB, "status_capamb", "", LDMS_V_U64, 0, 0 },
	[APP_STATUS_NONEWPRIVS] = { APP_STATUS_NONEWPRIVS, "status_nonewprivs", "", LDMS_V_U64, 0, 0 },
	[APP_STATUS_SECCOMP] = { APP_STATUS_SECCOMP, "status_seccomp", "", LDMS_V_U64, 0, 0 },
	[APP_STATUS_SPECULATION_STORE_BYPASS] = { APP_STATUS_SPECULATION_STORE_BYPASS, "status_speculation_store_bypass", "", LDMS_V_CHAR_ARRAY, SPECULATION_SZ, 0 },
	[APP_STATUS_CPUS_ALLOWED] = { APP_STATUS_CPUS_ALLOWED, "status_cpus_allowed", "", LDMS_V_U32_ARRAY, CPUS_ALLOWED_SZ, 0 },
	[APP_STATUS_CPUS_ALLOWED_LIST] = { APP_STATUS_CPUS_ALLOWED_LIST, "status_cpus_allowed_list", "", LDMS_V_CHAR_ARRAY, CPUS_ALLOWED_LIST_SZ, 0 },
	[APP_STATUS_MEMS_ALLOWED] = { APP_STATUS_MEMS_ALLOWED, "status_mems_allowed", "", LDMS_V_U32_ARRAY, MEMS_ALLOWED_SZ, 0 },
	[APP_STATUS_MEMS_ALLOWED_LIST] = { APP_STATUS_MEMS_ALLOWED_LIST, "status_mems_allowed_list", "", LDMS_V_CHAR_ARRAY, MEMS_ALLOWED_LIST_SZ, 0 },
	[APP_STATUS_VOLUNTARY_CTXT_SWITCHES] = { APP_STATUS_VOLUNTARY_CTXT_SWITCHES, "status_voluntary_ctxt_switches", "", LDMS_V_U64, 0, 0 },
	[APP_STATUS_NONVOLUNTARY_CTXT_SWITCHES] = { APP_STATUS_NONVOLUNTARY_CTXT_SWITCHES, "status_nonvoluntary_ctxt_switches", "", LDMS_V_U64, 0, 0 },

	[APP_SYSCALL] = { APP_SYSCALL, "syscall", "", LDMS_V_U64_ARRAY, 9, 0 },

	[APP_TIMERSLACK_NS] = { APP_TIMERSLACK_NS, "timerslack_ns", "ns", LDMS_V_U64, 0, 0 },

	[APP_WCHAN] = { APP_WCHAN, "wchan", "", LDMS_V_CHAR_ARRAY, WCHAN_SZ, 0 },

};

/* This array is initialized in __init__() below */
app_sampler_metric_info_t metric_info_idx_by_name[_APP_LAST + 1];

typedef struct app_sampler_inst_s *app_sampler_inst_t;
typedef int (*handler_fn_t)(app_sampler_inst_t inst, pid_t pid, ldms_set_t set);

static int cmdline_handler(app_sampler_inst_t, pid_t, ldms_set_t);
static int n_open_files_handler(app_sampler_inst_t, pid_t, ldms_set_t);
static int io_handler(app_sampler_inst_t, pid_t, ldms_set_t);
static int oom_score_handler(app_sampler_inst_t, pid_t, ldms_set_t);
static int oom_score_adj_handler(app_sampler_inst_t, pid_t, ldms_set_t);
static int root_handler(app_sampler_inst_t, pid_t, ldms_set_t);
static int stat_handler(app_sampler_inst_t, pid_t, ldms_set_t);
static int status_handler(app_sampler_inst_t, pid_t, ldms_set_t);
static int syscall_handler(app_sampler_inst_t, pid_t, ldms_set_t);
static int timerslack_ns_handler(app_sampler_inst_t, pid_t, ldms_set_t);
static int wchan_handler(app_sampler_inst_t, pid_t, ldms_set_t);

/* mapping metric -> handler */
handler_fn_t handler_tbl[] = {
	[_APP_CMDLINE_FIRST ... _APP_CMDLINE_LAST] = cmdline_handler,
	[APP_N_OPEN_FILES] = n_open_files_handler,
	[_APP_IO_FIRST ... _APP_IO_LAST] = io_handler,
	[APP_OOM_SCORE] = oom_score_handler,
	[APP_OOM_SCORE_ADJ] = oom_score_adj_handler,
	[APP_ROOT] = root_handler,
	[_APP_STAT_FIRST ... _APP_STAT_LAST] = stat_handler,
	[_APP_STATUS_FIRST ... _APP_STATUS_LAST] = status_handler,
	[APP_SYSCALL] = syscall_handler,
	[APP_TIMERSLACK_NS] = timerslack_ns_handler,
	[APP_WCHAN] = wchan_handler,
};

struct app_sampler_inst_s {
	struct ldmsd_plugin_inst_s base;

	char *stream_name;
	ldmsd_stream_client_t stream;

	handler_fn_t fn[16];
	int n_fn;

	int task_rank_idx;
	int metric_idx[_APP_LAST+1]; /* 0 means disabled */
};

static inline app_sampler_metric_info_t find_metric_info_by_name(const char *name);

/* ============ Handlers ============ */

/*
 * Read content of the file (given `path`) into string metric at `idx` in `set`,
 * with maximum length `max_len`.
 *
 * **REMARK**: The metric is not '\0'-terminated.
 */
static int __read_str(ldms_set_t set, int idx, const char *path, int max_len)
{
	int fd, rlen;
	ldms_mval_t str = ldms_metric_get(set, idx);
	fd = open(path, O_RDONLY);
	if (fd < 0)
		return errno;
	rlen = read(fd, str->a_char, max_len);
	close(fd);
	return rlen;
}

/* like `__read_str()`, but also '\0'-terminate the string */
static int __read_str0(ldms_set_t set, int idx, const char *path, int max_len)
{
	ldms_mval_t str = ldms_metric_get(set, idx);
	int rlen = __read_str(set, idx, path, max_len);
	if (rlen == max_len)
		rlen = max_len - 1;
	str->a_char[rlen] = '\0';
	return rlen;
}

/* Convenient functions that set `set[midx] = val` if midx > 0 */
static inline void __may_set_u64(ldms_set_t set, int midx, uint64_t val)
{
	if (midx > 0)
		ldms_metric_set_u64(set, midx, val);
}

static inline void __may_set_char(ldms_set_t set, int midx, char val)
{
	if (midx > 0)
		ldms_metric_set_char(set, midx, val);
}

static inline void __may_set_str(ldms_set_t set, int midx, const char *s)
{
	ldms_mval_t mval;
	int len;
	if (midx <= 0)
		return;
	mval = ldms_metric_get(set, midx);
	len = ldms_metric_array_get_len(set, midx);
	snprintf(mval->a_char, len, "%s", s);
}

static int cmdline_handler(app_sampler_inst_t inst, pid_t pid, ldms_set_t set)
{
	/* populate `cmdline` and `cmdline_len` */
	ldms_mval_t cmdline;
	int len;
	char path[4096];
	cmdline = ldms_metric_get(set, inst->metric_idx[APP_CMDLINE]);
	if (cmdline->a_char[0])
		return 0; /* already set */
	snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
	len = __read_str(set, inst->metric_idx[APP_CMDLINE], path, CMDLINE_SZ);
	cmdline->a_char[CMDLINE_SZ - 1] = 0; /* in case len == CMDLINE_SZ */
	ldms_metric_set_u64(set, inst->metric_idx[APP_CMDLINE_LEN], len);
	return 0;
}

static int n_open_files_handler(app_sampler_inst_t inst, pid_t pid,
				ldms_set_t set)
{
	/* populate n_open_files */
	char path[4096];
	DIR *dir;
	struct dirent *dent;
	int n;
	snprintf(path, sizeof(path), "/proc/%d/fd", pid);
	dir = opendir(path);
	if (!dir)
		return errno;
	n = 0;
	while ((dent = readdir(dir))) {
		if (strcmp(dent->d_name, ".") == 0)
			continue; /* skip self */
		if (strcmp(dent->d_name, "..") == 0)
			continue; /* skip parent */
		n += 1;
	}
	ldms_metric_set_u64(set, inst->metric_idx[APP_N_OPEN_FILES], n);
	closedir(dir);
	return 0;
}

static int io_handler(app_sampler_inst_t inst, pid_t pid, ldms_set_t set)
{
	/* populate io_* */
	char path[4096];
	uint64_t val[7];
	int n, rc;
	snprintf(path, sizeof(path), "/proc/%d/io", pid);
	FILE *f = fopen(path, "r");
	if (!f)
		return errno;
	n = fscanf(f,   "rchar: %lu\n"
			"wchar: %lu\n"
			"syscr: %lu\n"
			"syscw: %lu\n"
			"read_bytes: %lu\n"
			"write_bytes: %lu\n"
			"cancelled_write_bytes: %lu\n",
			val+0, val+1, val+2, val+3, val+4, val+5, val+6);
	if (n != 7) {
		rc = EINVAL;
		goto out;
	}
	__may_set_u64(set, inst->metric_idx[APP_IO_READ_B]           , val[0]);
	__may_set_u64(set, inst->metric_idx[APP_IO_WRITE_B]          , val[1]);
	__may_set_u64(set, inst->metric_idx[APP_IO_N_READ]           , val[2]);
	__may_set_u64(set, inst->metric_idx[APP_IO_N_WRITE]          , val[3]);
	__may_set_u64(set, inst->metric_idx[APP_IO_READ_DEV_B]       , val[4]);
	__may_set_u64(set, inst->metric_idx[APP_IO_WRITE_DEV_B]      , val[5]);
	__may_set_u64(set, inst->metric_idx[APP_IO_WRITE_CANCELLED_B], val[6]);
	rc = 0;
 out:
	fclose(f);
	return rc;
}

static int oom_score_handler(app_sampler_inst_t inst, pid_t pid, ldms_set_t set)
{
	/* according to `proc_oom_score()` in Linux kernel src tree, oom_score
	 * is `unsigned long` */
	char path[4096];
	uint64_t x;
	int n;
	snprintf(path, sizeof(path), "/proc/%d/oom_score", pid);
	FILE *f = fopen(path, "r");
	if (!f)
		return errno;
	n = fscanf(f, "%lu", &x);
	fclose(f);
	if (n != 1)
		return EINVAL;
	ldms_metric_set_u64(set, inst->metric_idx[APP_OOM_SCORE], x);
	return 0;
}

static int oom_score_adj_handler(app_sampler_inst_t inst, pid_t pid, ldms_set_t set)
{
	/* according to `proc_oom_score_adj_read()` in Linux kernel src tree,
	 * oom_score_adj is `short` */
	char path[4096];
	int x;
	int n;
	snprintf(path, sizeof(path), "/proc/%d/oom_score_adj", pid);
	FILE *f = fopen(path, "r");
	if (!f)
		return errno;
	n = fscanf(f, "%d", &x);
	fclose(f);
	if (n != 1)
		return EINVAL;
	ldms_metric_set_s64(set, inst->metric_idx[APP_OOM_SCORE_ADJ], x);
	return 0;
}

static int root_handler(app_sampler_inst_t inst, pid_t pid, ldms_set_t set)
{
	char path[256];
	ssize_t len;
	int midx = inst->metric_idx[APP_ROOT];
	assert(midx > 0);
	ldms_mval_t mval = ldms_metric_get(set, midx);
	int alen = ldms_metric_array_get_len(set, midx);
	/* /proc/<PID>/root is a soft link */
	snprintf(path, sizeof(path), "/proc/%d/root", pid);
	len = readlink(path, mval->a_char, alen - 1);
	if (len < 0) {
		mval->a_char[0] = '\0';
		return errno;
	}
	mval->a_char[len] = '\0';
	return 0;
}

static int stat_handler(app_sampler_inst_t inst, pid_t pid, ldms_set_t set)
{
	char buff[4096];
	char *str;
	char name[128]; /* should be enough to hold program name */
	int off, n;
	uint64_t val;
	char state;
	pid_t _pid;
	app_sampler_metric_e code;
	FILE *f;
	snprintf(buff, sizeof(buff), "/proc/%d/stat", pid);
	f = fopen(buff, "r");
	if (!f)
		return errno;
	fgets(buff, sizeof(buff), f);
	fclose(f);
	str = buff;
	n = sscanf(str, "%d (%[^)]) %c%n", &_pid, name, &state, &off);
	if (n != 3)
		return EINVAL;
	str += off;
	if (_pid != pid)
		return EINVAL; /* should not happen */
	__may_set_u64(set, inst->metric_idx[APP_STAT_PID], _pid);
	__may_set_str(set, inst->metric_idx[APP_STAT_COMM], name);
	__may_set_char(set, inst->metric_idx[APP_STAT_STATE], state);
	for (code = APP_STAT_PPID; code <= _APP_STAT_LAST; code++) {
		n = sscanf(str, "%lu%n", &val, &off);
		if (n != 1)
			return EINVAL;
		str += off;
		__may_set_u64(set, inst->metric_idx[code], val);
	}
	return 0;
}

typedef struct status_line_handler_s {
	const char *key;
	app_sampler_metric_e code;
	int (*fn)(app_sampler_inst_t inst, ldms_set_t set, const char *linebuf,
		  app_sampler_metric_e code);
} *status_line_handler_t;

int __line_dec(app_sampler_inst_t inst, ldms_set_t set, const char *linebuf, app_sampler_metric_e code);
int __line_dec_array(app_sampler_inst_t inst, ldms_set_t set, const char *linebuf, app_sampler_metric_e code);
int __line_hex(app_sampler_inst_t inst, ldms_set_t set, const char *linebuf, app_sampler_metric_e code);
int __line_oct(app_sampler_inst_t inst, ldms_set_t set, const char *linebuf, app_sampler_metric_e code);
int __line_char(app_sampler_inst_t inst, ldms_set_t set, const char *linebuf, app_sampler_metric_e code);
int __line_sigq(app_sampler_inst_t inst, ldms_set_t set, const char *linebuf, app_sampler_metric_e code);
int __line_str(app_sampler_inst_t inst, ldms_set_t set, const char *linebuf, app_sampler_metric_e code);
int __line_bitmap(app_sampler_inst_t inst, ldms_set_t set, const char *linebuf, app_sampler_metric_e code);

static status_line_handler_t find_status_line_handler(const char *key);

/* This table will later be sorted alphabetically in __init__() */
struct status_line_handler_s status_line_tbl[] = {
	/* linux/fs/proc/array.c:task_state() */
	{ "Name", APP_STATUS_NAME, __line_str},
	{ "Umask", APP_STATUS_UMASK, __line_oct},
	{ "State", APP_STATUS_STATE,__line_char },
	{ "Tgid", APP_STATUS_TGID, __line_dec},
	{ "Ngid", APP_STATUS_NGID, __line_dec},
	{ "Pid", APP_STATUS_PID, __line_dec},
	{ "PPid", APP_STATUS_PPID, __line_dec},
	{ "TracerPid", APP_STATUS_TRACERPID, __line_dec},
	{ "Uid", APP_STATUS_UID, __line_dec_array},
	{ "Gid", APP_STATUS_GID, __line_dec_array},
	{ "FDSize", APP_STATUS_FDSIZE, __line_dec},
	{ "Groups", APP_STATUS_GROUPS, __line_dec_array},
	{ "NStgid", APP_STATUS_NSTGID, __line_dec_array},
	{ "NSpid", APP_STATUS_NSPID, __line_dec_array},
	{ "NSpgid", APP_STATUS_NSPGID, __line_dec_array},
	{ "NSsid", APP_STATUS_NSSID, __line_dec_array},

	/* linux/fs/proc/task_mmu.c:task_mem() */
	{ "VmPeak", APP_STATUS_VMPEAK, __line_dec},
	{ "VmSize", APP_STATUS_VMSIZE, __line_dec},
	{ "VmLck", APP_STATUS_VMLCK, __line_dec},
	{ "VmPin", APP_STATUS_VMPIN, __line_dec},
	{ "VmHWM", APP_STATUS_VMHWM, __line_dec},
	{ "VmRSS", APP_STATUS_VMRSS, __line_dec},
	{ "RssAnon", APP_STATUS_RSSANON, __line_dec},
	{ "RssFile", APP_STATUS_RSSFILE, __line_dec},
	{ "RssShmem", APP_STATUS_RSSSHMEM, __line_dec},
	{ "VmData", APP_STATUS_VMDATA, __line_dec},
	{ "VmStk", APP_STATUS_VMSTK, __line_dec},
	{ "VmExe", APP_STATUS_VMEXE, __line_dec},
	{ "VmLib", APP_STATUS_VMLIB, __line_dec},
	{ "VmPTE", APP_STATUS_VMPTE, __line_dec},
	{ "VmPMD", APP_STATUS_VMPMD, __line_dec},
	{ "VmSwap", APP_STATUS_VMSWAP, __line_dec},
	{ "HugetlbPages", APP_STATUS_HUGETLBPAGES, __line_dec},

	/* linux/fs/proc/array.c:task_core_dumping() */
	{ "CoreDumping", APP_STATUS_COREDUMPING, __line_dec},

	/* linux/fs/proc/array.c:task_sig() */
	{ "Threads", APP_STATUS_THREADS, __line_dec},
	{ "SigQ", APP_STATUS_SIG_QUEUED, __line_sigq},
	{ "SigPnd", APP_STATUS_SIGPND, __line_hex},
	{ "ShdPnd", APP_STATUS_SHDPND, __line_hex},
	{ "SigBlk", APP_STATUS_SIGBLK, __line_hex},
	{ "SigIgn", APP_STATUS_SIGIGN, __line_hex},
	{ "SigCgt", APP_STATUS_SIGCGT, __line_hex},

	/* linux/fs/proc/array.c:task_cap() */
	{ "CapInh", APP_STATUS_CAPINH, __line_hex},
	{ "CapPrm", APP_STATUS_CAPPRM, __line_hex},
	{ "CapEff", APP_STATUS_CAPEFF, __line_hex},
	{ "CapBnd", APP_STATUS_CAPBND, __line_hex},
	{ "CapAmb", APP_STATUS_CAPAMB, __line_hex},

	/* linux/fs/proc/array.c:task_seccomp() */
	{ "NoNewPrivs", APP_STATUS_NONEWPRIVS, __line_dec},
	{ "Seccomp", APP_STATUS_SECCOMP, __line_dec},
	{ "Speculation_Store_Bypass", APP_STATUS_SPECULATION_STORE_BYPASS, __line_str},

	/* linux/fs/proc/array.c:task_cpus_allowed() */
	{ "Cpus_allowed", APP_STATUS_CPUS_ALLOWED, __line_bitmap},
	{ "Cpus_allowed_list", APP_STATUS_CPUS_ALLOWED_LIST, __line_str},

	/* linux/kernel/cgroup/cpuset.c:cpuset_task_status_allowed() */
	{ "Mems_allowed", APP_STATUS_MEMS_ALLOWED, __line_bitmap},
	{ "Mems_allowed_list", APP_STATUS_MEMS_ALLOWED_LIST, __line_str},

	/* linux/fs/proc/array.c:task_context_switch_counts() */
	{ "voluntary_ctxt_switches", APP_STATUS_VOLUNTARY_CTXT_SWITCHES, __line_dec},
	{ "nonvoluntary_ctxt_switches", APP_STATUS_NONVOLUNTARY_CTXT_SWITCHES, __line_dec},
};

static int status_handler(app_sampler_inst_t inst, pid_t pid, ldms_set_t set)
{
	char buff[4096];
	FILE *f;
	char *line, *key, *ptr;
	status_line_handler_t sh;

	snprintf(buff, sizeof(buff), "/proc/%d/status", pid);
	f = fopen(buff, "r");
	if (!f)
		return errno;
	while ((line = fgets(buff, sizeof(buff), f))) {
		key = strtok_r(line, ":", &ptr);
		sh = find_status_line_handler(key);
		if (!sh)
			continue;
		while (isspace(*ptr)) {
			ptr++;
		}
		ptr[strlen(ptr)-1] = '\0'; /* eliminate trailing space */
		if (inst->metric_idx[sh->code] > 0
				|| sh->code == APP_STATUS_SIG_QUEUED) {
			sh->fn(inst, set, ptr, sh->code);
		}
	}
	fclose(f);
	return 0;
}

int __line_dec(app_sampler_inst_t inst, ldms_set_t set,
		const char *linebuf, app_sampler_metric_e code)
{
	/* scan for a uint64_t */
	int n;
	uint64_t x;
	n = sscanf(linebuf, "%ld", &x);
	if (n != 1)
		return EINVAL;
	ldms_metric_set_u64(set, inst->metric_idx[code], x);
	return 0;
}

int __line_dec_array(app_sampler_inst_t inst, ldms_set_t set,
		const char *linebuf, app_sampler_metric_e code)
{
	int i, n, alen, pos;
	int midx = inst->metric_idx[code];
	const char *ptr = linebuf;
	uint64_t val;
	alen = ldms_metric_array_get_len(set, midx);
	for (i = 0; i < alen; i++) {
		n = sscanf(ptr, "%lu%n", &val, &pos);
		if (n != 1)
			break;
		ldms_metric_array_set_u64(set, midx, i, val);
		ptr += pos;
	}
	return 0;
}

int __line_hex(app_sampler_inst_t inst, ldms_set_t set,
		const char *linebuf, app_sampler_metric_e code)
{
	int n;
	uint64_t x;
	n = sscanf(linebuf, "%lx", &x);
	if (n != 1)
		return EINVAL;
	ldms_metric_set_u64(set, inst->metric_idx[code], x);
	return 0;
}

int __line_oct(app_sampler_inst_t inst, ldms_set_t set,
		const char *linebuf, app_sampler_metric_e code)
{
	int n;
	uint64_t x;
	n = sscanf(linebuf, "%lo", &x);
	if (n != 1)
		return EINVAL;
	ldms_metric_set_u64(set, inst->metric_idx[code], x);
	return 0;
}

int __line_char(app_sampler_inst_t inst, ldms_set_t set,
		const char *linebuf, app_sampler_metric_e code)
{
	int n;
	char x;
	n = sscanf(linebuf, "%c", &x);
	if (n != 1)
		return EINVAL;
	ldms_metric_set_char(set, inst->metric_idx[code], x);
	return 0;
}

int __line_sigq(app_sampler_inst_t inst, ldms_set_t set,
		const char *linebuf, app_sampler_metric_e code)
{
	uint64_t q, l;
	int n;
	n = sscanf(linebuf, "%lu/%lu", &q, &l);
	if (n != 2)
		return EINVAL;
	__may_set_u64(set, inst->metric_idx[APP_STATUS_SIG_QUEUED], q);
	__may_set_u64(set, inst->metric_idx[APP_STATUS_SIG_LIMIT] , l);
	return 0;
}

int __line_str(app_sampler_inst_t inst, ldms_set_t set, const char *linebuf,
		app_sampler_metric_e code)
{
	int midx = inst->metric_idx[code];
	int alen = ldms_metric_array_get_len(set, midx);
	ldms_mval_t mval = ldms_metric_get(set, midx);
	snprintf(mval->a_char, alen, "%s", linebuf);
	ldms_metric_array_set_char(set, midx, alen-1, '\0'); /* to update data generation number */
	return 0;
}

int __line_bitmap(app_sampler_inst_t inst, ldms_set_t set, const char *linebuf,
		  app_sampler_metric_e code)
{
	/* line format: <4-byte-hex>,<4-byte-hex>,...<4-byte-hex>
	 * line ordering: high-byte ... low-byte
	 * ldms array ordering: low-byte .. high-byte
	 * high-byte will be truncated if the ldms array is too short.
	 */
	const char *s;
	int n = strlen(linebuf);
	int midx = inst->metric_idx[code];
	int alen = ldms_metric_array_get_len(set, midx);
	int i, val;
	for (i = 0; i < alen && n; i++) {
		/* reverse scan */
		s = memrchr(linebuf, ',', n);
		if (!s) {
			s = linebuf;
			sscanf(s, "%x", &val);
		} else {
			sscanf(s, ",%x", &val);
		}
		ldms_metric_array_set_u32(set, midx, i, val);
		n = s - linebuf;
	}
	return 0;
}

static int syscall_handler(app_sampler_inst_t inst, pid_t pid, ldms_set_t set)
{
	char buff[4096];
	FILE *f;
	char *ret;
	int i, n;
	uint64_t val[9] = {0};
	snprintf(buff, sizeof(buff), "/proc/%d/syscall", pid);
	/*
	 * NOTE: The file contains single line wcich could be:
	 * - "running": the process is running.
	 * - "-1 <STACK_PTR> <PROGRAM_CTR>": the process is blocked, but not in
	 *   a syscall.
	 * - "<SYSCALL_NUM> <ARG0> ... <ARG5> <STACK_PTR> <PROGRAM_CTR>": the
	 *   syscall number, 6 arguments, stack pointer and program counter.
	 */
        f = fopen(buff, "r");
        if (!f)
		return errno;
	ret = fgets(buff, sizeof(buff), f);
	fclose(f);
	if (!ret)
		return errno;
	if (0 == strncmp(buff, "running", 7)) {
		n = 0;
	} else {
		n = sscanf(buff, "%ld %lx %lx %lx %lx %lx %lx %lx %lx",
				 val+0, val+1, val+2, val+3, val+4,
				 val+5, val+6, val+7, val+8);
	}
	for (i = 0; i < n; i++) {
		ldms_metric_array_set_u64(set, inst->metric_idx[APP_SYSCALL],
					  i, val[i]);
	}
	for (i = n; i < 9; i++) {
		/* zero */
		ldms_metric_array_set_u64(set, inst->metric_idx[APP_SYSCALL],
					  i, 0);
	}
	return 0;
}

static int timerslack_ns_handler(app_sampler_inst_t inst, pid_t pid, ldms_set_t set)
{
	char path[4096];
	uint64_t x;
	int n;
	snprintf(path, sizeof(path), "/proc/%d/timerslack_ns", pid);
	FILE *f = fopen(path, "r");
	if (!f)
		return errno;
	n = fscanf(f, "%lu", &x);
	fclose(f);
	if (n != 1)
		return EINVAL;
	ldms_metric_set_u64(set, inst->metric_idx[APP_TIMERSLACK_NS], x);
	return 0;
}

static int wchan_handler(app_sampler_inst_t inst, pid_t pid, ldms_set_t set)
{
	char path[4096];
	snprintf(path, sizeof(path), "/proc/%d/wchan", pid);
	__read_str0(set, inst->metric_idx[APP_WCHAN], path, WCHAN_SZ);
	return 0;
}


/* ============== Sampler Plugin APIs ================= */

static
int app_sampler_update_schema(ldmsd_plugin_inst_t pi, ldms_schema_t schema)
{
	app_sampler_inst_t inst = (void*)pi;
	int i, idx;
	app_sampler_metric_info_t mi;

	inst->task_rank_idx = ldms_schema_meta_add(schema, "task_rank",
					LDMS_V_U64, "");

	/* Add app metrics to the schema */
	for (i = 1; i <= _APP_LAST; i++) {
		if (!inst->metric_idx[i])
			continue;
		mi = &metric_info[i];
		if (ldms_type_is_array(mi->mtype)) {
			if (mi->is_meta) {
				idx = ldms_schema_meta_array_add(schema,
						mi->name, mi->mtype, mi->unit,
						mi->array_len);
			} else {
				idx = ldms_schema_metric_array_add(schema,
						mi->name, mi->mtype, mi->unit,
						mi->array_len);
			}
		} else {
			if (mi->is_meta) {
				idx = ldms_schema_meta_add(schema, mi->name,
						mi->mtype, mi->unit);
			} else {
				idx = ldms_schema_metric_add(schema, mi->name,
						mi->mtype, mi->unit);
			}
		}
		inst->metric_idx[i] = idx;
	}
	return 0;
}

static
int app_sampler_update_set(ldmsd_plugin_inst_t pi, ldms_set_t set, void *ctxt)
{
	app_sampler_inst_t inst = (void*)pi;
	pid_t pid = (uint64_t)ctxt; /* pid stored in ctxt when set is created */
	int i;
	for (i = 0; i < inst->n_fn; i++) {
		inst->fn[i](inst, pid, set);
	}
	return 0;
}


/* ============== Common Plugin APIs ================= */

static
const char *app_sampler_desc(ldmsd_plugin_inst_t pi)
{
	return "app_sampler - sampling /proc/PID/ files for Slurm job";
}

static
char *_help = "\
app_sampler config synopsis: \
    config name=INST [COMMON_OPTIONS] [stream=STREAM] [metrics=METRICS] \n\
           [cfg_file=FILE]\n\
\n\
Option descriptions:\n\
    stream    The name of the `ldmsd_stream` to listen for SLURM job events.\n\
              (default: slurm).\n\
    metrics   The comma-separated list of metrics to monitor.\n\
              The default is "" (empty), which is equivalent to monitor ALL\n\
              metrics.\n\
    cfg_file  The alternative config file in JSON format. The file is\n\
              expected to have an object that contains the following \n\
              attributes:\n\
              - \"stream\": \"STREAM_NAME\"\n\
              - \"metrics\": [ METRICS ]\n\
              If the `cfg_file` is given, `stream` and `metrics` options\n\
              are ignored.\n\
\n\
The sampler creates and destroys sets according to events received from \n\
LDMSD stream. The sets share the same schema which is contructed according \n\
to the given `metrics` option or \"metrics\" in the JSON `cfg_file`.\n\
The sampling frequency is controlled by LDMSD `smplr` object.\n\
\n\
";

static
const char *app_sampler_help(ldmsd_plugin_inst_t pi)
{
	return _help;
}

static
int __handle_cfg_file(app_sampler_inst_t inst, json_entity_t jval,
		      char *ebuf, int ebufsz)
{
	int i, fd, rc = -1, off;
	ssize_t sz;
	char *buff = NULL;
	json_parser_t jp = NULL;
	json_entity_t jdoc = NULL;
	json_entity_t list, ent;
	app_sampler_metric_info_t minfo;

	if (jval->type != JSON_STRING_VALUE) {
		snprintf(ebuf, ebufsz, "Error: cfg_file must be a string.");
		return EINVAL;
	}
	fd = open(jval->value.str_->str, O_RDONLY);
	if (fd < 0)
		return errno;
	sz = lseek(fd, 0, SEEK_END);
	if (sz < 0) {
		rc = errno;
		snprintf(ebuf, ebufsz, "lseek() failed, errno: %d", errno);
		goto out;
	}
	lseek(fd, 0, SEEK_SET);
	buff = malloc(sz);
	if (!buff) {
		rc = errno;
		snprintf(ebuf, ebufsz, "Out of memory");
		goto out;
	}
	off = 0;
	while (sz) {
		rc = read(fd, buff + off, sz);
		if (rc < 0) {
			rc = errno;
			snprintf(ebuf, ebufsz, "read() error: %d", errno);
			goto out;
		}
		off += rc;
		sz -= rc;
	}

	jp = json_parser_new(0);
	if (!jp) {
		rc = errno;
		snprintf(ebuf, ebufsz, "json_parser_new() error: %d", errno);
		goto out;
	}
	rc = json_parse_buffer(jp, buff, sz, &jdoc);
	if (rc) {
		snprintf(ebuf, ebufsz, "JSON parse failed: %d", rc);
		goto out;
	}

	ent = json_value_find(jdoc, "stream");
	if (ent) {
		if (ent->type != JSON_STRING_VALUE) {
			rc = EINVAL;
			snprintf(ebuf, ebufsz, "Error: `stream` must be a string.");
			goto out;
		}
		inst->stream_name = strdup(ent->value.str_->str);
		if (!inst->stream_name) {
			rc = ENOMEM;
			snprintf(ebuf, ebufsz, "Out of memory.");
			goto out;
		}
	} /* else, caller will later set default stream_name */
	list = json_value_find(jdoc, "metrics");
	if (list) {
		for (ent = json_item_first(list); ent;
					ent = json_item_next(ent)) {
			if (ent->type != JSON_STRING_VALUE) {
				rc = EINVAL;
				snprintf(ebuf, ebufsz,
					 "Error: metric must be a string.");
				goto out;
			}
			minfo = find_metric_info_by_name(ent->value.str_->str);
			if (!minfo) {
				rc = ENOENT;
				snprintf(ebuf, ebufsz,
					 "Error: metric '%s' not found",
					 ent->value.str_->str);
				goto out;
			}
			inst->metric_idx[minfo->code] = -1;
			/* Non-zero to enable. This will be replaced with the
			 * actual metric index later. */
		}
	} else {
		for (i = _APP_FIRST; i <= _APP_LAST; i++) {
			inst->metric_idx[i] = -1;
		}
	}
	rc = 0;

 out:
	close(fd);
	if (buff)
		free(buff);
	if (jp)
		json_parser_free(jp);
	if (jdoc)
		json_entity_free(jdoc);
	return rc;
}

int __handle_task_init(app_sampler_inst_t inst, json_entity_t data)
{
	ldmsd_sampler_type_t samp = LDMSD_SAMPLER(inst);
	ldms_set_t set;
	int len;
	json_entity_t job_id;
	json_entity_t task_pid;
	json_entity_t task_rank;
	char *setname;
	job_id = json_value_find(data, "job_id");
	if (!job_id || job_id->type != JSON_INT_VALUE)
		return EINVAL;
	task_pid = json_value_find(data, "task_pid");
	if (!task_pid || task_pid->type != JSON_INT_VALUE)
		return EINVAL;
	task_rank = json_value_find(data, "task_global_id");
	if (!task_rank || task_rank->type != JSON_INT_VALUE)
		return EINVAL;
	len = asprintf(&setname, "%s/%ld/%ld",
			samp->producer_name,
			job_id->value.int_,
			task_pid->value.int_);
	if (len < 0)
		return ENOMEM;
	set = samp->create_set(LDMSD_INST(inst), setname, samp->schema,
				(void*)task_pid->value.int_);
	free(setname);
	if (!set)
		return errno;
	ldms_metric_set_u64(set, SAMPLER_JOB_IDX, job_id->value.int_);
	if (samp->component_id)
		ldms_metric_set_u64(set, SAMPLER_COMP_IDX, samp->component_id);
	ldms_metric_set_u64(set, inst->task_rank_idx, task_rank->value.int_);
	/* leave other data to be filled in update_set() */
	return 0;
}

int __handle_task_exit(app_sampler_inst_t inst, json_entity_t data)
{
	ldmsd_sampler_type_t samp = LDMSD_SAMPLER(inst);
	ldms_set_t set;
	int len;
	json_entity_t job_id;
	json_entity_t task_pid;
	char *setname;
	job_id = json_value_find(data, "job_id");
	if (!job_id || job_id->type != JSON_INT_VALUE)
		return EINVAL;
	task_pid = json_value_find(data, "task_pid");
	if (!task_pid || task_pid->type != JSON_INT_VALUE)
		return EINVAL;
	len = asprintf(&setname, "%s/%ld/%ld",
			samp->producer_name,
			job_id->value.int_,
			task_pid->value.int_);
	if (len < 0)
		return ENOMEM;
	set = ldms_set_by_name(setname);
	free(setname);
	if (!set)
		return errno;
	samp->delete_set(LDMSD_INST(inst), set);
	ldms_set_put(set); /* put ref from `ldms_set_by_name()` */
	return 0;
}

int __stream_cb(ldmsd_stream_client_t c, void *ctxt,
		ldmsd_stream_type_t stream_type,
		const char *msg, size_t msg_len, json_entity_t entity)
{
	app_sampler_inst_t inst = ctxt;
	json_entity_t event, data;
	const char *event_name;

	if (stream_type != LDMSD_STREAM_JSON) {
		INST_LOG(inst, LDMSD_LDEBUG, "Unexpected stream type data...ignoring\n");
		INST_LOG(inst, LDMSD_LDEBUG, "%s\n", msg);
		return EINVAL;
	}

	event = json_value_find(entity, "event");
	if (!event) {
		INST_LOG(inst, LDMSD_LERROR, "'event' attribute missing\n");
		goto out_0;
	}
	if (event->type != JSON_STRING_VALUE) {
		INST_LOG(inst, LDMSD_LERROR, "'event' is not a string\n");
		goto out_0;
	}
	event_name = event->value.str_->str;
	data = json_value_find(entity, "data");
	if (!data) {
		INST_LOG(inst, LDMSD_LERROR,
			 "'%s' event is missing the 'data' attribute\n",
			 event_name);
		goto out_0;
	}
	if (0 == strcmp(event_name, "task_init_priv")) {
		return __handle_task_init(inst, data);
	} else if (0 == strcmp(event_name, "task_exit")) {
		return __handle_task_exit(inst, data);
	}
 out_0:
	return 0;
}

static
int app_sampler_config(ldmsd_plugin_inst_t pi, json_entity_t json,
				      char *ebuf, int ebufsz)
{
	app_sampler_inst_t inst = (void*)pi;
	ldmsd_sampler_type_t samp = (void*)inst->base.base;
	int i, rc;
	json_entity_t jval;
	app_sampler_metric_info_t minfo;

	rc = samp->base.config(pi, json, ebuf, ebufsz);
	if (rc)
		return rc;

	/* Plugin-specific config here */
	jval = json_value_find(json, "cfg_file");
	if (jval) {
		rc = __handle_cfg_file(inst, jval, ebuf, ebufsz);
		if (rc)
			return rc;
	} else {
		jval = json_value_find(json, "stream");
		if (jval) {
			inst->stream_name = strdup(jval->value.str_->str);
			if (!inst->stream_name) {
				snprintf(ebuf, ebufsz, "Out of memory");
				return ENOMEM;
			}
		}
		jval = json_value_find(json, "metrics");
		if (jval) {
			char *tkn, *ptr;
			tkn = strtok_r(jval->value.str_->str, ",", &ptr);
			while (tkn) {
				minfo = find_metric_info_by_name(tkn);
				if (!minfo) {
					snprintf(ebuf, ebufsz,
						 "Error: metric '%s' not found",
						 tkn);
					return ENOENT;
				}
				inst->metric_idx[minfo->code] = -1;
				tkn = strtok_r(NULL, ",", &ptr);
			}
		} else {
			for (i = _APP_FIRST; i <= _APP_LAST; i++) {
				inst->metric_idx[i] = -1;
			}
		}
	}

	/* default stream */
	if (!inst->stream_name) {
		inst->stream_name = strdup("slurm");
		if (!inst->stream_name) {
			snprintf(ebuf, ebufsz, "Out of memory");
			return ENOMEM;
		}
	}

	inst->n_fn = 0;
	for (i = _APP_FIRST; i <= _APP_LAST; i++) {
		if (inst->metric_idx[i] == 0)
			continue;
		if (inst->n_fn && inst->fn[inst->n_fn-1] == handler_tbl[i])
			continue; /* already added */
		/* add the handler */
		inst->fn[inst->n_fn] = handler_tbl[i];
		inst->n_fn++;
	}

	/* create schema */
	samp->schema = samp->create_schema(pi);
	if (!samp->schema) {
		snprintf(ebuf, ebufsz, "Out of memory");
		return errno;
	}

	/* subscribe to the stream */
	inst->stream = ldmsd_stream_subscribe(inst->stream_name, __stream_cb, inst);
	if (!inst->stream) {
		snprintf(ebuf, ebufsz, "Error subcribing to stream `%s`: %d",
			 inst->stream_name, errno);
		return errno;
	}

	return 0;
}

static
void app_sampler_del(ldmsd_plugin_inst_t pi)
{
	app_sampler_inst_t inst = (void*)pi;

	ldmsd_stream_close(inst->stream);
	free(inst->stream_name);
}

static
int app_sampler_base_update_set(ldmsd_plugin_inst_t inst, ldms_set_t set)
{
	ldmsd_sampler_type_t samp = LDMSD_SAMPLER(inst);
	if (samp->component_id && !ldms_metric_get_u64(set, SAMPLER_COMP_IDX)) {
		/* set component_id only if it has not been set */
		ldms_metric_set_u64(set, SAMPLER_COMP_IDX, samp->component_id);
	}
	return 0;
}

static
int app_sampler_init(ldmsd_plugin_inst_t pi)
{
	app_sampler_inst_t inst = (void*)pi;
	ldmsd_sampler_type_t samp = (void*)inst->base.base;

	samp->update_schema = app_sampler_update_schema;
	/* override base_update_set to only update `component_id` part.
	 * The default `job_id` assignment is replaced by the slurm job_id we
	 * get from task_priv_init event in __stream_cb() */
	samp->base_update_set = app_sampler_base_update_set;
	samp->update_set = app_sampler_update_set;

	return 0;
}

static
struct app_sampler_inst_s __inst = {
	.base = {
		.version     = LDMSD_PLUGIN_VERSION_INITIALIZER,
		.type_name   = LDMSD_SAMPLER_TYPENAME,
		.plugin_name = "app_sampler",

                /* Common Plugin APIs */
		.desc   = app_sampler_desc,
		.help   = app_sampler_help,
		.init   = app_sampler_init,
		.del    = app_sampler_del,
		.config = app_sampler_config,

	},
};

ldmsd_plugin_inst_t new()
{
	app_sampler_inst_t inst = malloc(sizeof(*inst));
	if (inst)
		*inst = __inst;
	return &inst->base;
}

static int
idx_cmp_by_name(const void *a, const void *b)
{
	/* a, b are pointer to app_sampler_metric_info_t */
	const app_sampler_metric_info_t *_a = a, *_b = b;
	return strcmp((*_a)->name, (*_b)->name);
}

static int
idx_key_cmp_by_name(const void *key, const void *ent)
{
	const app_sampler_metric_info_t *_ent = ent;
	return strcmp(key, (*_ent)->name);
}

static inline app_sampler_metric_info_t
find_metric_info_by_name(const char *name)
{
	app_sampler_metric_info_t *minfo = bsearch(name, metric_info_idx_by_name,
			_APP_LAST + 1, sizeof(app_sampler_metric_info_t),
			idx_key_cmp_by_name);
	return minfo?(*minfo):(NULL);
}

static int
status_line_cmp(const void *a, const void *b)
{
	const struct status_line_handler_s *_a = a, *_b = (void*)b;
	return strcmp(_a->key, _b->key);
}

static int
status_line_key_cmp(const void *key, const void *ent)
{
	const struct status_line_handler_s *_ent = ent;
	return strcmp(key, _ent->key);
}

static status_line_handler_t
find_status_line_handler(const char *key)
{
	return bsearch(key, status_line_tbl, ARRAY_LEN(status_line_tbl),
			sizeof(status_line_tbl[0]), status_line_key_cmp);
}

__attribute__((constructor))
static
void __init__()
{
	/* initialize metric_info_idx_by_name */
	int i;
	for (i = 0; i <= _APP_LAST; i++) {
		metric_info_idx_by_name[i] = &metric_info[i];
	}
	qsort(metric_info_idx_by_name, _APP_LAST + 1,
		sizeof(metric_info_idx_by_name[0]), idx_cmp_by_name);
	qsort(status_line_tbl, ARRAY_LEN(status_line_tbl),
			sizeof(status_line_tbl[0]), status_line_cmp);
}

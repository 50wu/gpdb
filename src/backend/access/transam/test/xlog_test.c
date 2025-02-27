#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmockery.h"

#include "../xlog.c"

static void
KeepLogSeg_wrapper(XLogRecPtr recptr, XLogSegNo *logSegNo)
{
	KeepLogSeg(recptr, logSegNo, InvalidXLogRecPtr, false);
}

static void
test_KeepLogSeg(void **state)
{
	XLogRecPtr recptr;
	XLogSegNo  _logSegNo;
	XLogCtlData xlogctl;

	xlogctl.replicationSlotMinLSN = InvalidXLogRecPtr;
	SpinLockInit(&xlogctl.info_lck);
	XLogCtl = &xlogctl;

	/*
	 * 64 segments per Xlog logical file.
	 * Configuring (3, 2), 3 log files and 2 segments to keep (3*64 + 2).
	 */
	wal_keep_segments = 194;

	/*
	 * Set wal segment size to 64 mb
	 */
	wal_segment_size = 64 * 1024 * 1024;

	/************************************************
	 * Current Delete greater than what keep wants,
	 * so, delete offset should get updated
	 ***********************************************/
	/* Current Delete pointer */
	_logSegNo = 3 * XLogSegmentsPerXLogId(wal_segment_size) + 10;

	/*
	 * Current xlog location (4, 1)
	 * xrecoff = seg * 67108864 (64 MB segsize)
	 */
	recptr = ((uint64) 4) << 32 | (wal_segment_size * 1);

	KeepLogSeg_wrapper(recptr, &_logSegNo);
	assert_int_equal(_logSegNo, 63);
	/************************************************/


	/************************************************
	 * Current Delete smaller than what keep wants,
	 * so, delete offset should NOT get updated
	 ***********************************************/
	/* Current Delete pointer */
	_logSegNo = 60;

	/*
	 * Current xlog location (4, 1)
	 * xrecoff = seg * 67108864 (64 MB segsize)
	 */
	recptr = ((uint64) 4) << 32 | (wal_segment_size * 1);

	KeepLogSeg_wrapper(recptr, &_logSegNo);
	assert_int_equal(_logSegNo, 60);
	/************************************************/


	/************************************************
	 * Current Delete smaller than what keep wants,
	 * so, delete offset should NOT get updated
	 ***********************************************/
	/* Current Delete pointer */
	_logSegNo = 1 * XLogSegmentsPerXLogId(wal_segment_size) + 60;

	/*
	 * Current xlog location (5, 8)
	 * xrecoff = seg * 67108864 (64 MB segsize)
	 */
	recptr = ((uint64) 5) << 32 | (wal_segment_size * 8);

	KeepLogSeg_wrapper(recptr, &_logSegNo);
	assert_int_equal(_logSegNo, 1 * XLogSegmentsPerXLogId(wal_segment_size) + 60);
	/************************************************/

	/************************************************
	 * UnderFlow case, curent is lower than keep
	 ***********************************************/
	/* Current Delete pointer */
	_logSegNo = 2 * XLogSegmentsPerXLogId(wal_segment_size) + 1;

	/*
	 * Current xlog location (3, 1)
	 * xrecoff = seg * 67108864 (64 MB segsize)
	 */
	recptr = ((uint64) 3) << 32 | (wal_segment_size * 1);

	KeepLogSeg_wrapper(recptr, &_logSegNo);
	assert_int_equal(_logSegNo, 1);
	/************************************************/

	/************************************************
	 * One more simple scenario of updating delete offset
	 ***********************************************/
	/* Current Delete pointer */
	_logSegNo = 2 * XLogSegmentsPerXLogId(wal_segment_size) + 8;

	/*
	 * Current xlog location (5, 8)
	 * xrecoff = seg * 67108864 (64 MB segsize)
	 */
	recptr = ((uint64) 5) << 32 | (wal_segment_size * 8);

	KeepLogSeg_wrapper(recptr, &_logSegNo);
	assert_int_equal(_logSegNo, 2*XLogSegmentsPerXLogId(wal_segment_size) + 6);
	/************************************************/

	/************************************************
	 * Do nothing if wal_keep_segments is not positive
	 ***********************************************/
	/* Current Delete pointer */
	wal_keep_segments = 0;
	_logSegNo = 9 * XLogSegmentsPerXLogId(wal_segment_size) + 45;

	KeepLogSeg_wrapper(recptr, &_logSegNo);
	assert_int_equal(_logSegNo, 9*XLogSegmentsPerXLogId(wal_segment_size) + 45);

	wal_keep_segments = -1;

	KeepLogSeg_wrapper(recptr, &_logSegNo);
	assert_int_equal(_logSegNo, 9*XLogSegmentsPerXLogId(wal_segment_size) + 45);
	/************************************************/
}

int
main(int argc, char* argv[])
{
	cmockery_parse_arguments(argc, argv);

	const UnitTest tests[] = {
		unit_test(test_KeepLogSeg)
	};
	return run_tests(tests);
}

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "loopback.h"
#include "glib.h"
#include "purple.h"
#include "purple_defs.h"

guint purple_timeout_add_test(guint interval, GSourceFunc function, gpointer data)
{
	return 0xABCD1234;
}

gboolean purple_timeout_remove_test(guint handle)
{
	CPPUNIT_ASSERT(handle == 0xABCD1234);
}

class MessageLoopbackTrackerSpecimen : public MessageLoopbackTracker {
public:
	guint timer() { return this->m_timer; }
};


class MessageLoopbackTrackerTest : public CPPUNIT_NS :: TestFixture {
	CPPUNIT_TEST_SUITE(MessageLoopbackTrackerTest);
	CPPUNIT_TEST(matchIdentical);
	CPPUNIT_TEST(mismatchDifferent);
	CPPUNIT_TEST(mismatchPast);
	CPPUNIT_TEST(mismatchFuture);
	CPPUNIT_TEST(matchOnlyOnce);
	CPPUNIT_TEST(matchTwice);
	CPPUNIT_TEST(trim);
	CPPUNIT_TEST(purpleUiOps);
	CPPUNIT_TEST(cleanupByTimeout);
	CPPUNIT_TEST(autotrimEnableDisable);
	CPPUNIT_TEST_SUITE_END();

	protected:
		MessageLoopbackTrackerSpecimen *m_tracker;
		PurpleEventLoopUiOps ops;

	public:
		void setUp (void) {
#if PURPLE_RUNTIME
			//Set fake timer functions
			purple_timeout_add_wrapped = &purple_timeout_add_wrapped_test;
			purple_timeout_remove_wrapped = &purple_timeout_remove_wrapped_test;
#else
			//Register in a proper way
			memset(&ops, 0, sizeof(ops));
			ops.timeout_add = purple_timeout_add_test;
			ops.timeout_remove = purple_timeout_remove_test;
			purple_eventloop_set_ui_ops_wrapped(&ops);
#endif
			m_tracker = new MessageLoopbackTrackerSpecimen();
			CPPUNIT_ASSERT(m_tracker->timer() == 0);
			m_tracker->setAutotrim(false);
		}

		void tearDown (void) {
			delete m_tracker;
		}

		PurpleConversation* conv(int seed) {
			return reinterpret_cast<PurpleConversation*>(seed);
		}

	void matchIdentical() {
		m_tracker->add(conv(0x12345678), "Test message text 1");
		CPPUNIT_ASSERT(m_tracker->matchAndRemove(conv(0x12345678), "Test message text 1", time(0)));
	}

	void mismatchDifferent() {
		m_tracker->add(conv(0x12345678), "Test message text 2");
		//Changing any of the parameters should result in mismatch:
		CPPUNIT_ASSERT(!m_tracker->matchAndRemove(conv(0x12345679), "Test message text 2", time(0)));
		CPPUNIT_ASSERT(!m_tracker->matchAndRemove(conv(0x12345678), "Test message text 22", time(0)));
		CPPUNIT_ASSERT(!m_tracker->matchAndRemove(conv(0x12345678), "Test message text ", time(0)));
		//We should still match the original message after these mismatches:
		CPPUNIT_ASSERT(m_tracker->matchAndRemove(conv(0x12345678), "Test message text 2", time(0)));
	}

	void mismatchPast() {
		m_tracker->add(conv(0x12345678), "Test message text 3");
		//A match from the past should not work
		CPPUNIT_ASSERT(!m_tracker->matchAndRemove(conv(0x12345678), "Test message text 3", time(0)-1));
	}

	void mismatchFuture() {
		m_tracker->add(conv(0x12345678), "Test message text 4");
		//A match from too far in the future should not work
		CPPUNIT_ASSERT(!m_tracker->matchAndRemove(conv(0x12345678), "Test message text 4", time(0)+MessageLoopbackTracker::CarbonTimeout+1));
	}

	void matchOnlyOnce() {
		m_tracker->add(conv(0x12345678), "Test message text 6");
		//Each message instance should be matched only once
		CPPUNIT_ASSERT(m_tracker->matchAndRemove(conv(0x12345678), "Test message text 6", time(0)));
		CPPUNIT_ASSERT(!m_tracker->matchAndRemove(conv(0x12345678), "Test message text 6", time(0)));
	}

	void matchTwice() {
		//If we add the same message twice then it should be matched twice, but no more
		m_tracker->add(conv(0x12345678), "Test message text 7");
		m_tracker->add(conv(0x12345678), "Test message text 7");
		CPPUNIT_ASSERT(m_tracker->matchAndRemove(conv(0x12345678), "Test message text 7", time(0)));
		CPPUNIT_ASSERT(m_tracker->matchAndRemove(conv(0x12345678), "Test message text 7", time(0)));
		CPPUNIT_ASSERT(!m_tracker->matchAndRemove(conv(0x12345678), "Test message text 7", time(0)));
	}

	void trim() {
		//Trimmed messages should not be matched
		m_tracker->add(conv(0x12345678), "Test message text 8");
		m_tracker->trim(time(0)+1);
		CPPUNIT_ASSERT(!m_tracker->matchAndRemove(conv(0x12345678), "Test message text 8", time(0)));
	}

	void purpleUiOps() {
		guint timer = purple_timeout_add(10000, NULL, NULL);
		purple_timeout_remove(timer);
	}

	void cleanupByTimeout() {
		//We can't do real timeout testing here as that would require
		//1. Waiting for that time to pass
		//2. Having libpurple initialized
	}
	
	void autotrimEnableDisable() {
		//Enable and disable autotrim multiple times
		m_tracker->setAutotrim(true);
		m_tracker->setAutotrim(false);
		CPPUNIT_ASSERT(m_tracker->timer() == 0);
		m_tracker->setAutotrim(false); //again
		m_tracker->setAutotrim(true);
		m_tracker->setAutotrim(false);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION (MessageLoopbackTrackerTest);

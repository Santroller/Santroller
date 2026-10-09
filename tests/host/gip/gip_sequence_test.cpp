// lib/gip_common/gip_sequence.h: the per-message sequence ID pools the USB host uses for what it
// sends to a controller. [MS-GIPUSB] 2.2.10.3 / Table 15: Protocol Control, Hello, Status,
// Metadata, Set Device State, Guide Button Status, Audio Control, LED Guide Button and Large
// Message / Debug share the global pool; Security (0x06), Extended Commands (0x1E), Audio
// (0x60) and every vendor message (e.g. 0x09, 0x20, 0x26) use their own. 2.2.10: "0x00 is
// reserved for both system and vendor messages".
#include <gtest/gtest.h>

#include "gip_sequence.h"

TEST(GipSequencePool, StartsAtOne)
{
    gip_sequence_pool_t pool;
    gip_sequence_pool_init(&pool);
    for (uint8_t cmd : {0x01, 0x04, 0x05, 0x06, 0x09, 0x1E, 0x20, 0x22, 0x26, 0x60})
    {
        gip_sequence_pool_t fresh = pool;
        EXPECT_EQ(gip_sequence_pool_next(&fresh, cmd), 1) << (int)cmd;
    }
}

TEST(GipSequencePool, GlobalMessagesShareOneCounter)
{
    gip_sequence_pool_t pool;
    gip_sequence_pool_init(&pool);
    const uint8_t global[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x07, 0x08, 0x0A, 0x1F};
    uint8_t expected = 1;
    for (uint8_t cmd : global)
    {
        EXPECT_TRUE(gip_is_global_sequence_pool(cmd)) << (int)cmd;
        EXPECT_EQ(gip_sequence_pool_next(&pool, cmd), expected++) << (int)cmd;
    }
}

TEST(GipSequencePool, UniquePoolsAreIndependent)
{
    gip_sequence_pool_t pool;
    gip_sequence_pool_init(&pool);
    // Advance the global pool a few times; none of the unique pools move
    for (int i = 0; i < 5; i++)
        gip_sequence_pool_next(&pool, 0x05);
    for (uint8_t cmd : {0x06, 0x1E, 0x60, 0x09, 0x20, 0x26, 0x22})
    {
        EXPECT_FALSE(gip_is_global_sequence_pool(cmd)) << (int)cmd;
        EXPECT_EQ(gip_sequence_pool_next(&pool, cmd), 1) << (int)cmd;
        EXPECT_EQ(gip_sequence_pool_next(&pool, cmd), 2) << (int)cmd;
    }
    EXPECT_EQ(gip_sequence_pool_next(&pool, 0x05), 6);
}

TEST(GipSequencePool, WrapSkipsZero)
{
    gip_sequence_pool_t pool;
    gip_sequence_pool_init(&pool);
    uint8_t last = 0;
    for (int i = 0; i < 600; i++)
    {
        uint8_t seq = gip_sequence_pool_next(&pool, GIP_AUTH);
        EXPECT_NE(seq, 0);
        if (last == 0xFF)
        {
            EXPECT_EQ(seq, 1);
        }
        else if (last)
        {
            EXPECT_EQ(seq, last + 1);
        }
        last = seq;
    }
}

TEST(GipSequencePool, CurrentDoesNotAdvance)
{
    gip_sequence_pool_t pool;
    gip_sequence_pool_init(&pool);
    gip_sequence_pool_next(&pool, GIP_CMD_RUMBLE);
    EXPECT_EQ(gip_sequence_pool_current(&pool, GIP_CMD_RUMBLE), 2);
    EXPECT_EQ(gip_sequence_pool_current(&pool, GIP_CMD_RUMBLE), 2);
    EXPECT_EQ(gip_sequence_pool_next(&pool, GIP_CMD_RUMBLE), 2);
}

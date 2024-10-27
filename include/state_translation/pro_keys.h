
// Translate a Pro keys style report to a usb host report
// Note that we temporarily clear the buttons and then merge after
// As that makes it easier to process the velocities
#define TRANSLATE_PRO_KEYS(SRC, DEST)              \
    DEST->a |= SRC->a;                             \
    DEST->b |= SRC->b;                             \
    DEST->x |= SRC->x;                             \
    DEST->y |= SRC->y;                             \
    DEST->key1 |= SRC->key1;                       \
    DEST->key2 |= SRC->key2;                       \
    DEST->key3 |= SRC->key3;                       \
    DEST->key4 |= SRC->key4;                       \
    DEST->key5 |= SRC->key5;                       \
    DEST->key6 |= SRC->key6;                       \
    DEST->key7 |= SRC->key7;                       \
    DEST->key8 |= SRC->key8;                       \
                                                   \
    DEST->key9 |= SRC->key9;                       \
    DEST->key10 |= SRC->key10;                     \
    DEST->key11 |= SRC->key11;                     \
    DEST->key12 |= SRC->key12;                     \
    DEST->key13 |= SRC->key13;                     \
    DEST->key14 |= SRC->key14;                     \
    DEST->key15 |= SRC->key15;                     \
    DEST->key16 |= SRC->key16;                     \
                                                   \
    DEST->key17 |= SRC->key17;                     \
    DEST->key18 |= SRC->key18;                     \
    DEST->key19 |= SRC->key19;                     \
    DEST->key20 |= SRC->key20;                     \
    DEST->key21 |= SRC->key21;                     \
    DEST->key22 |= SRC->key22;                     \
    DEST->key23 |= SRC->key23;                     \
    DEST->key24 |= SRC->key24;                     \
    DEST->key25 |= SRC->key25;                     \
    DEST->overdrive |= SRC->overdrive;             \
    DEST->pedal |= SRC->pedal;                     \
    if (SRC->pedalAnalog) {                        \
        DEST->pedalAnalog = SRC->pedalAnalog << 1; \
    }                                              \
    if (SRC->touchPad) {                           \
        DEST->touchPad = SRC->touchPad << 1;       \
    }                                              \
    if (SRC->velocities[0]) {                      \
        DEST->velocities[0] = SRC->velocities[0];  \
    }                                              \
    if (SRC->velocities[1]) {                      \
        DEST->velocities[1] = SRC->velocities[1];  \
    }                                              \
    if (SRC->velocities[2]) {                      \
        DEST->velocities[2] = SRC->velocities[2];  \
    }                                              \
    if (SRC->velocities[3]) {                      \
        DEST->velocities[3] = SRC->velocities[3];  \
    }                                              \
    if (SRC->velocities[4]) {                      \
        DEST->velocities[4] = SRC->velocities[4];  \
    }

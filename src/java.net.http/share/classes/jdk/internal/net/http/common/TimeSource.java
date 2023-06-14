/*
 * Copyright (c) 2023, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */
package jdk.internal.net.http.common;

import java.time.Instant;
import java.time.InstantSource;

/**
 * An {@link InstantSource} based on {@link System#nanoTime()} for the
 * purpose of handling timeouts. This time source is intentionally not
 * based on the system clock, in order to not be sensitive to clock skews
 * caused by changes to the wall clock. Consequently, callers should use
 * instants returned by this time source solely for the purpose of
 * comparing them with other instants returned by this same time source.
 * The granularity of instants returned is identical to the granularity
 * of {@link System#nanoTime()}. This time source has the same property
 * of monotonicity than the {@link System#nanoTime()} it is based on.
 */
public final class TimeSource implements InstantSource {

    private static volatile NanoSource nanoSource = new NanoSource();
    private static final TimeSource SOURCE = new TimeSource();

    private NanoSource localSource = nanoSource;
    private TimeSource() {}

    private static final class NanoSource {
        // Duration (in nanoseconds) for which the static nanoSource
        // instance is considered valid.
        // The use of Integer.MAX_VALUE is arbitrary.
        // Any value not too close to Long.MAX_VALUE
        // would do.
        static final int TIME_WINDOW = Integer.MAX_VALUE;

        final Instant first;
        final long firstNanos;
        NanoSource() {
            this(Instant.now(), System.nanoTime());
        }
        NanoSource(Instant first, long firstNanos) {
            this.first = first;
            this.firstNanos = firstNanos;
        }
        
        Instant instant(long nanos) {
            return instant(nanos, nanos - firstNanos);
        }

        Instant instant(long nanos, long delay) {
            Instant now = first.plusNanos(delay);
            if (!isInWindow(delay)) {
                // Shifts the time reference (firstNanos) to
                // prevent issues that may be caused by
                // System.nanoTime() - firstNanos wrapping
                // around.
                nanoSource = new NanoSource(now, nanos);
            }
            return now;
        }

        long delay(long nanos) {
            return nanos - firstNanos;
        }

        boolean isInWindow(long delay) {
            return delay >= 0 && delay <= TIME_WINDOW;
        }

    }

    @Override
    public Instant instant() {
        long nanos = System.nanoTime();
        long delay = localSource.delay(nanos);
        // use localSource if possible to avoid a volatile read
        if (localSource.isInWindow(delay)) {
            return localSource.instant(nanos, delay);
        } else {
            // will cause the time reference to shift forward,
            // at the cost of a volatile write + a volatile read
            var instant = nanoSource.instant(nanos);
            // if this is reordered by the compiler it's not a big deal,
            // will get the new value next time around...
            localSource = nanoSource;
            return instant;
        }
    }

    /**
     * {@return the time source}
     */
    public static TimeSource source() {
        return SOURCE;
    }

    /**
     * {@return the current instant obtained from the time source}
     * This is equivalent to calling:
     * {@snippet
     *     TimeSource.source().instant();
     * }
     */
    public static Instant now() {
        return SOURCE.instant();
    }
}

/*
 * Copyright (c) 2001, 2023, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
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

/*
  @test
  @bug 4394789
  @summary KeyboardFocusManager.upFocusCycle is not working for Swing properly
  @key headful
  @run main UpFocusCycleTest
*/
import java.awt.BorderLayout;
import java.awt.Container;
import java.awt.Color;
import java.awt.DefaultKeyboardFocusManager;
import java.awt.EventQueue;
import java.awt.event.FocusAdapter;
import java.awt.event.FocusEvent;
import java.awt.Robot;
import javax.swing.DefaultFocusManager;
import javax.swing.JButton;
import javax.swing.JFrame;

public class UpFocusCycleTest {
    static boolean isFailed = true;
    static Object sema = new Object();
    static JFrame frame;

    public static void main(String[] args) throws Exception {
        try {
            Robot robot = new Robot();
            EventQueue.invokeAndWait(() -> {

                frame = new JFrame("Test frame");

                Container container1 = frame.getContentPane();
                container1.setBackground(Color.yellow);

                JButton button = new JButton("Button");
                button.addFocusListener(new FocusAdapter() {
                    public void focusGained(FocusEvent fe) {
                        DefaultKeyboardFocusManager manager = new DefaultFocusManager();
                        manager.upFocusCycle(button);
                        System.out.println("Button receive focus");
                        frame.addFocusListener(new FocusAdapter() {
                            public void focusGained(FocusEvent fe) {
                                System.out.println("Frame receive focus");
                                synchronized (sema) {
                                    isFailed = false;
                                    sema.notifyAll();
                                }
                            }
                        });
                    }
                });
                container1.add(button,BorderLayout.WEST);
                button.requestFocus();
                frame.setSize(300,300);
                frame.setLocationRelativeTo(null);
                frame.setVisible(true);
            });
            robot.waitForIdle();
            robot.delay(1000);
            if (isFailed) {
                System.out.println("Test FAILED");
                throw new RuntimeException("Test FAILED");
            } else {
                System.out.println("Test PASSED");
            }
        } finally {
            if (frame != null) {
                frame.dispose();
            }
        }
    }
 }// class UpFocusCycleTest

/*
 * Copyright (C) 2018 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */
package com.huawei.rtcore.vktriangle;

import android.app.NativeActivity;
import android.os.Bundle;

public class VulkanActivity extends NativeActivity {

    static {
        // Load native library
        System.loadLibrary("native-lib");
    }
    // create window
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

}

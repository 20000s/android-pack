package com.android.second;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.Gravity;
import android.widget.FrameLayout;
import android.widget.TextView;

public class MainActivity2 extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        TextView tv= new TextView(this);
        // 创建布局设置参数
        FrameLayout.LayoutParams params=new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.WRAP_CONTENT,
                FrameLayout.LayoutParams.WRAP_CONTENT
        );
        // 设置控件到顶点
        params.topMargin=0;
        // 设置控件的位置
        params.gravity= Gravity.TOP|Gravity.CENTER_HORIZONTAL;
        tv.setText("MyTestActivity Textview!");
        // 添加TextView到Activity
        this.addContentView(tv,params);
    }
}
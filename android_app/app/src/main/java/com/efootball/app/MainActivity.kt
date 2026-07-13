package com.efootball.app

import android.os.Bundle
import android.widget.Button
import android.widget.LinearLayout
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import java.io.File

class MainActivity : AppCompatActivity() {

    private lateinit var statusText: TextView
    private lateinit var startButton: Button
    private lateinit var stopButton: Button

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        val layout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(64, 64, 64, 64)
        }

        statusText = TextView(this).apply {
            text = "Bot Status: Stopped"
            textSize = 24f
            setPadding(0, 0, 0, 64)
        }
        layout.addView(statusText)

        startButton = Button(this).apply {
            text = "Start Bot"
            setOnClickListener {
                val dbFile = File(filesDir, "efootball.db")
                startBot(dbFile.absolutePath)
                statusText.text = "Bot Status: Running"
                isEnabled = false
                stopButton.isEnabled = true
            }
        }
        layout.addView(startButton)

        stopButton = Button(this).apply {
            text = "Stop Bot"
            isEnabled = false
            setOnClickListener {
                stopBot()
                statusText.text = "Bot Status: Stopped"
                isEnabled = false
                startButton.isEnabled = true
            }
        }
        layout.addView(stopButton)

        setContentView(layout)
    }

    /**
     * A native method that is implemented by the 'efootballbot' native library.
     */
    external fun startBot(dbPath: String)
    external fun stopBot()

    companion object {
        init {
            System.loadLibrary("efootballbot")
        }
    }
}

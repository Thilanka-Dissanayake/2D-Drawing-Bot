# 🎨 2D Drawing Bot (CNC Pen Plotter)

An automated 2D drawing machine capable of translating digital vector graphics into physical sketches. This project bridges digital image processing with embedded hardware control, utilizing precise motor movements to draw complex designs on paper.

## ⚙️ The Pipeline (How It Works)
1.  **Digital Processing:** An image is converted into a vector format (SVG) and translated into G-code (machine coordinates).
2.  **Transmission:** The G-code instructions are sent to the microcontroller via serial communication.
3.  **Execution:** The microcontroller parses the commands and precisely drives the X and Y axis stepper motors while toggling a servo motor to lift and drop the pen.

## 🛠️ Hardware & Tech Stack

### Components
*   **Microcontroller:** ( ATmega328p)
*   **Motors:** Stepper Motors ( 28BYJ-48) for X/Y axes, Micro Servo for Z-axis (pen lift)
*   **Motor Drivers:** (A4988)
*   **Chassis/Mechanics:** ( 3D printed parts, aluminum extrusions)

### Software & Languages
*   **Embedded Code:** C++ / GRBL Firmware
*   **Image Processing:** Python (for SVG/G-code generation)
*   **G-code Sender:** (Custom Python script)

## 🚀 Setup & Execution
1.  Assemble the hardware chassis and wire the stepper drivers 
2.  Flash the firmware to your microcontroller.
3.  Generate your G-code from an SVG file.
4.  Run the serial communication script or open your G-code sender to begin plotting!

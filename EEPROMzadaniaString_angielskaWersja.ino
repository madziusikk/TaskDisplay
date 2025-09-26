#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial BTSerial(10, 11); // RX, TX 

const int MAX_TASKS = 20;  // Maximum number of tasks saved to EEPROM (hardware limitations of Arduino)
const int MAX_TEXT_LENGTH = 47;  // Length of the text
const int TASK_BLOCK_SIZE = 50;  // 2 bytes for metadata + 50 for the text
const int redDiodePin = A0;
const int greenDiodePin = A1;
char taskText[MAX_TEXT_LENGTH + 1];
int slotFilled[MAX_TASKS] = {0}; 


  // Data storage schema
    // Address | Content
    // 0       | 4 (text length)
    // 1       | 1 (task number)
    // 2       | 1 (isTaskCompleted)
    // 3       | 'T'
    // 4       | 'e'
    // 5       | 's'
    // 6       | 't'



// Helper function to calculate the base/start address for a task
int getTaskBaseAddress(int taskNumber) {
  return (taskNumber - 1) * TASK_BLOCK_SIZE;
}

// The saveText() function stores the text of a task into EEPROM memory.
// It saves the text length, task number, and task completion status as metadata, 
// then stores each character of the task text into consecutive EEPROM memory addresses.
void saveText(int taskNumber, const char* text) {
  if (taskNumber < 1 || taskNumber > MAX_TASKS) {
    Serial.println(F("Invalid task number.\n"));
    return;
  }

  int textLength = strlen(text);
  if (textLength > MAX_TEXT_LENGTH) {
    Serial.println(F("Invalid task number.\n"));
    return;
  }

  int baseAddress = getTaskBaseAddress(taskNumber);
  
  // Save text length and task number
  EEPROM.write(baseAddress, textLength);
  EEPROM.write(baseAddress + 1, taskNumber);
  EEPROM.write(baseAddress + 2, 0);

  // Save task text
  for (int i = 0; i < textLength; i++) {
    EEPROM.write(baseAddress + 3 + i, text[i]);
  }

  slotFilled[taskNumber - 1] = 1;

  Serial.print(taskNumber);
  Serial.print(". ");
  Serial.print(text);
  Serial.print(".");
  Serial.println("");

}

// The readText() function retrieves the stored task from EEPROM based on the given task number.
// It reads the text length, task number, and task status as metadata, 
// then retrieves the task text and displays it.
void readText(int taskNumber) {
  if (taskNumber < 1 || taskNumber > MAX_TASKS) {
    Serial.println(F("Invalid task number.\n"));
    Serial.println(" ");
    return;
  }

  int baseAddress = getTaskBaseAddress(taskNumber);
  
  // Read text length and task number
  int textLength = EEPROM.read(baseAddress);
  int taskNum = EEPROM.read(baseAddress + 1);
  int taskStatus = EEPROM.read(baseAddress + 2);

  /// Check if task number matches
  if (taskNum != taskNumber) {
    Serial.println("Task not found!\n");

    return;
  }

  char taskText[MAX_TEXT_LENGTH + 1];
  for (int i = 0; i < textLength; i++) {
    taskText[i] = EEPROM.read(baseAddress + 3 + i);
  }
  taskText[textLength] = '\0';

  Serial.print(taskNumber);
  Serial.print(". ");
  Serial.print(taskText);
  Serial.println(" ");
  if (taskStatus == 1) {
  Serial.println(F("Task status: done.\n"));
  }
  else {
  Serial.println(F("Task status: inProgress.\n"));
  }

  
}

// The changeTaskStatus() function updates the status of a task in EEPROM memory.
// It can set the task status as 'done' or 'in progress'.
void changeTaskStatus(int taskNumber, int taskStatus) {
    int baseAddress = getTaskBaseAddress(taskNumber);
    Serial.print("Task ");
    Serial.print(taskNumber);
    Serial.print(" has been mark as");
    if (taskStatus ==1) {
      Serial.println(" 'done'.");
      EEPROM.write(baseAddress + 2, 1);
    }
    else {
      Serial.println(" 'in progress'.");
      EEPROM.write(baseAddress + 2, 0);
    }
}

// The helpBar() function displays a help message with information about the system and available commands.
// It also explains the limitations on task numbers and task text length.
void helpBar() {
  Serial.println(F( "\n---------------------- INFO -------------------------"));
  Serial.print(F("Remember, each task can have up to "));
  Serial.print(MAX_TEXT_LENGTH);
  Serial.println(F(" characters."));
  Serial.print(F("The system supports up to "));
  Serial.print(MAX_TASKS);
  Serial.println(F(" tasks. Adding more is not possible due to hardware limitations."));
  Serial.println(F( "-------------------- COMMANDS LIST --------------------"));
  Serial.println(F("'add XX - Text'                  adds new task."));
  Serial.println(F("'read XX'                        reads added task."));
  Serial.println(F("'mark XX - done/inProgress'      sets new status to the task"));
  Serial.println(F("'delete XX'                      deletes added task."));
  Serial.println(F("'reset'                          clears all stored tasks from memory."));
  Serial.print(F("[XX- number of task between 01 to "));
  Serial.print(MAX_TASKS);
  Serial.println(F("]\n\n"));
}

// The displayAllTasks() function displays all tasks stored in the EEPROM memory.
// It checks each slot for filled status and reads the task information if it's filled.
void displayAllTasks() {
  Serial.println("List:");
  for (int i = 0; i < MAX_TASKS; i++) {
    if (slotFilled[i] == 1) {
      readText(i + 1);
    }
  }
}

// The deleteTask() function deletes a specific task from EEPROM memory.
// It clears the task's metadata (text length, task number, and status) and marks the slot as empty.
// Parameters:
void deleteTask(int taskNumber) {
  if (taskNumber < 1 || taskNumber > MAX_TASKS) {
        Serial.println(F("Invalid task number.\n"));
        return;
    }
    int baseAddress = getTaskBaseAddress(taskNumber);
    if(EEPROM.read(baseAddress)==0 && EEPROM.read(baseAddress + 1)==0 && EEPROM.read(baseAddress + 2)==0){
      Serial.println(F("The task was previously deleted.\n"));
    }
    else {
      EEPROM.write(baseAddress, 0);     // Reset text length
      EEPROM.write(baseAddress + 1, 0); // Reset task number
      EEPROM.write(baseAddress + 2, 0); // Reset task status
      slotFilled[taskNumber-1] = 0;
      Serial.print(F("Task "));
      Serial.print(taskNumber);
      Serial.println(F(" has been deleted successfully.\n"));
    }
    
}

// The resetEEPROM() function clears all tasks from EEPROM memory.
// It resets each task's metadata (text length, task number, and status) and marks all slots as empty.
void resetEEPROM() {
  for (int i = 0; i < MAX_TASKS; i++) {
    int baseAddress = getTaskBaseAddress(i + 1);
    EEPROM.write(baseAddress, 0);     // Reset text length
    EEPROM.write(baseAddress + 1, 0); // Reset task number
    EEPROM.write(baseAddress + 2, 0); // Reset task status
    slotFilled[i] = 0;
  }
  Serial.println(F("EEPROM memory has been reset successfully.\n"));
}

// The hardResetEEPROM() function completely clears all tasks in EEPROM memory.
// It overwrites all task data with zeros (by writing 0 to all memory locations).
void hardResetEEPROM() {
  Serial.println("Reseting ...");
  for (int i = 0; i < MAX_TASKS; i++) {
    int baseAddress = getTaskBaseAddress(i + 1);

    // Reset all task data by writing 0
    for (int j = 0; j < TASK_BLOCK_SIZE; j++) {
      EEPROM.write(baseAddress + j, 0);
    }

    // Set slotFilled[i] to 0
    slotFilled[i] = 0; 
  }
    Serial.println("EEPROM hard reset completed. All data has been erased.");  
    Serial.println("EEPROM memory has been restored to default settings.\n");
}

// The displayTasksOnScreen() function shows the tasks on the LCD screen.
// It retrieves the task information and displays it, showing whether the task is done or in progress.
void displayTasksOnScreen() {
  lcd.setCursor(1,0);
  lcd.print(F("Today's tasks"));
  delay(1500);
  
  for (int i = 0; i < MAX_TASKS; i++) {
    if (slotFilled[i] == 1) {
      int baseAddress = getTaskBaseAddress(i + 1);
      int textLength = EEPROM.read(baseAddress);
      int taskStatus = EEPROM.read(baseAddress + 2);
      
      for (int j = 0; j < textLength; j++) {
        taskText[j] = EEPROM.read(baseAddress + 3 + j);
      }
      taskText[textLength] = '\0';

      if (taskStatus == 1)
      {
        analogWrite(redDiodePin, 0);
        analogWrite(greenDiodePin, 255);
      }
      else {
        analogWrite(redDiodePin, 255);
        analogWrite(greenDiodePin, 255);
      }
      // If the text is longer than 30 characters (16 + 14), we will scroll it
      if (strlen(taskText) > 30) {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(i + 1);
        lcd.print(". ");

        // Display static text for a moment
        if (strlen(taskText) > 14) {
          // Search for the last space before the 14th character
          int lastSpace = 13;
          while (lastSpace >= 0 && taskText[lastSpace] != ' ') {
            lastSpace--;
          }
          
          if (lastSpace <= 0) {
            lastSpace = 14;
          }

          char firstLine[15];
          strncpy(firstLine, taskText, lastSpace);
          firstLine[lastSpace] = '\0';
          lcd.print(firstLine);
          
          lcd.setCursor(0,1);
          lcd.print(taskText + lastSpace + (taskText[lastSpace] == ' ' ? 1 : 0));
        } else {
          lcd.print(taskText);
        }
        delay(2000);
        lcd.clear();
        // Scroll the text
        for (int startPos = 0; startPos < strlen(taskText)- 29; startPos++) {
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print(i + 1);
          lcd.print(". ");
          
          // First line
          char line1[15];
          strncpy(line1, taskText + startPos, 14);
          line1[14] = '\0';
          lcd.print(line1);
          
          // Second line
          lcd.setCursor(0,1);
          if (startPos + 14 < strlen(taskText)) {
            char line2[17];
            strncpy(line2, taskText + startPos + 14, 16);
            line2[16] = '\0';
            lcd.print(line2);
          }
          
          delay(500);  // Scrolling speed
        }
        delay(1500);  // Pause at the end of scrolling
        lcd.clear();
      } 
      else {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(i + 1);
        lcd.print(". ");

        if (strlen(taskText) > 13) {
          // Search for the last space before the 13th character
          int lastSpace = 13;
          while (lastSpace >= 0 && taskText[lastSpace] != ' ') {
            lastSpace--;
          }
          
          if (lastSpace <= 0) {
            lastSpace = 14;
          }

          char firstLine[15];
          strncpy(firstLine, taskText, lastSpace);
          firstLine[lastSpace] = '\0';
          lcd.print(firstLine);
          
          lcd.setCursor(0,1);
          lcd.print(taskText + lastSpace + (taskText[lastSpace] == ' ' ? 1 : 0));
        } else {
          lcd.print(taskText);
        }
        
        delay(3000);  lcd.clear();
      }
    }
  }
  digitalWrite(redDiodePin, LOW);
  digitalWrite(greenDiodePin, LOW);
}


void setup() {
  BTSerial.begin(34800);  
  Serial.begin(9600);
  lcd.init();  
  lcd.backlight();

  analogWrite(redDiodePin, 0);
  analogWrite(greenDiodePin, 0);
  pinMode(9, OUTPUT);    /* this pin will pull the HC-05 pin 34 (KEY pin) HIGH to switch module to AT mode */
  digitalWrite(9, HIGH); 

   for (int i = 0; i < MAX_TASKS; i++) {
    int baseAddress = getTaskBaseAddress(i + 1);
    int taskNum = EEPROM.read(baseAddress + 1);
    if (taskNum == (i + 1)) {                        // This loop initializes the slotFilled array by checking stored task numbers in EEPROM.
      slotFilled[i] = 1;                             // It verifies which task slots are occupied and marks them accordingly.  
    } else {
      slotFilled[i] = 0;
    }
  }
  Serial.println(F("\nWelcome in TODO!"));
  Serial.println(F("Type 'help' to see the available commands.\n"));
  BTSerial.println(F("\nWelcome in TODO!"));
  BTSerial.println(F("Type 'help' to see the available commands.\n"));
}


void loop() {
  displayTasksOnScreen();
  if (BTSerial.available()) {
    String input = BTSerial.readString();
    Serial.print(input);
    input.trim();

    if (input.equalsIgnoreCase("hardReset")) {
    hardResetEEPROM();
    }
    else if (input.equalsIgnoreCase("reset")) {
      resetEEPROM();
    }
    else if (input.equalsIgnoreCase("help")){
    helpBar();
    }
    else if (input.equalsIgnoreCase("list")){
    displayAllTasks();
    }
    else if (input.startsWith("add")) {
      int separatorIndex = input.indexOf('-');
      if (separatorIndex > 4) {
        String taskNumberString = input.substring(4, separatorIndex - 1);
        taskNumberString.trim();
        int taskNumber = taskNumberString.toInt();
        String taskText = input.substring(separatorIndex + 1);
        taskText.trim();
        if (taskNumber >= 1 && taskNumber <= MAX_TASKS && taskText.length() <= 46 && taskText.length()>0) {
          saveText(taskNumber, taskText.c_str());
          Serial.println(F("Task saved.\n"));
          delay(100);
        } else {
          Serial.println(F("Error: Task number must be between 01 and 20, and text can have up to 46 characters.\n"));
        }
      } else {
        Serial.println(F("Error: The correct format is: add XX - Task text.\n"));
      }
    }
    else if (input.startsWith("read")) {
      String taskNumberString = input.substring(4);
      taskNumberString.trim();
      int taskNumber = taskNumberString.toInt();
      if (taskNumber >= 1 && taskNumber <=MAX_TASKS) {
        readText(taskNumber);
      }
      else {
        Serial.println(F("Error: Task number must be between 01 and 20.\n"));

      }
    }
    else if (input.startsWith("del")) {
      String taskNumberString = input.substring(3);
      taskNumberString.trim();
      int taskNumber = taskNumberString.toInt();
      if (taskNumber >= 1 && taskNumber <=MAX_TASKS) {
        deleteTask(taskNumber);
      }
      else {
        Serial.println(F("Error: Task number must be between 01 and 20.\n"));
      };
    }
    else if (input.startsWith("mark")) {
      int separatorIndex = input.indexOf('-');
      if (separatorIndex > 3) {
        String taskNumberString = input.substring(5, separatorIndex);
        taskNumberString.trim();
        int taskNumber = taskNumberString.toInt();
        String taskStatus = input.substring(separatorIndex + 1);
        taskStatus.trim();
        
        if (taskNumber >= 1 && taskNumber <= MAX_TASKS) {
          if (taskStatus == "done") {
            changeTaskStatus(taskNumber, 1);
          }
          else if (taskStatus == "inProgress") {
            changeTaskStatus(taskNumber, 0);
          }
          else {
            Serial.println(F("Error: Invalid status. Use 'done' or 'inProgress'.\n"));
          }
        }
        else {
          Serial.println(F("Error: Task number must be between 01 and 20.\n"));
        }
      }
      else {
        Serial.println(F("Error: The correct format is: mark XX - done/inProgress\n"));
      }
    }
    else {
      Serial.println(F("Invalid command provided. Please use the correct format.\n"));

    }
  }
}
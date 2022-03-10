#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>

//Read
bool qurysearch(){
  String tmp = "";
  int qrow = 0;
  bool getqury = false;
  char QUERY_SQL[] = "select * from lora_local.send where nodemcu_get ='0' LIMIT 1 ";

  // MYSQL checker
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  if (conn.connected())
  {
  }
  else
  {
    conn.close();
    //Serial.println("Connecting...");
    delay(200);
    if (conn.connect(server_addr, 3306, user, pass))
    {
      MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
      delay(500);
      Serial.println("Successful reconnect!");
    }
    else
    {
      Serial.println("Cannot reconnect!");
      return getqury;
    }
  }

  //Creat MySQL Connect Item
  cur_mem->execute(QUERY_SQL);
  //get columns
  column_names *cols = cur_mem->get_columns();
  row_values *row = NULL;
  do
  {
    row = cur_mem->get_next_row();
    if (row != NULL)
    {
      for (int f = 0; f < cols->num_fields; f++)
      {
        Serial.print(row->values[f]);
        quryArray[qrow][f] = row->values[f];
        if (f < cols->num_fields - 1)
        {
          Serial.print(',');
        }
      }
      qrow = qrow + 1;
      Serial.println();
      getqury = true;
    }
  } while (row != NULL);
  // 刪除mysql實例爲下次採集作準備
  delete cur_mem;

  return getqury;
}



void quryupdate(String updateID)
{
  String QUERY_SQL_update = "UPDATE lora_local.send SET nodemcu_get='1' WHERE id='" + updateID + "'";
  char Buf[500];
  QUERY_SQL_update.toCharArray(Buf, 500);
  Serial.println(Buf);

  // Creat MySQL Connect Item
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  if (conn.connected())
  {
    // do something your work
    cur_mem->execute(Buf);
    delete cur_mem;
    Serial.println("UPDATE success");
  }
  else
  {
    conn.close();
    Serial.println("Connecting...");
    delay(200); //
    if (conn.connect(server_addr, 3306, user, pass))
    {
      delay(500);
      Serial.println("Successful reconnect!");
      // Insert into DB
      Serial.println("UPDATE INTO DB");
      Serial.println(Buf);
      cur_mem->execute(Buf);
      delete cur_mem;
      Serial.println("UPDATE success");
    }
    else
    {
      Serial.println("Cannot reconnect! Drat.");
    }
  }
}

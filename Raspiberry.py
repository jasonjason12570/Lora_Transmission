import serial
import time
import string
import mysql.connector
from mysql.connector import Error
keyword = "LORA"

#Check Module check new Data
def searchDB():
    #DB Get config
	dhost = "192.168.31.200" #serverip
	ddb = "lora_local"
	duser = "localuser"
	dpass = "qweasdzxc"
	try:
    #DB connection
    conn = mysql.connector.connect(
        host = dhost,
        database = ddb,
        user = duser,
        password = dpass
    )
    #DB Get SQL
    sql = "select * from predict where PCcheck='1' and checkType != '' and RPIcheck=''"
    cursor = conn.cursor()
    cursor.execute(sql)
    result = cursor.fetchall()
    cursor.close()
    conn.close()
    if(result==[]):
        return 0
    else:
        return result
  except Error as e:
      print("DB connect Error")

#Data collect Module
def data_get(obj):
  #########GET ALL DATA about Temp+Humi+GPS FROM Sensor#########

  #########GET GPS#########
  port="/dev/ttyAMA0"
  ser=serial.Serial(port, baudrate=9600, timeout=0.5)
  except_counter = 0
  sender = "26008625"
  while 1:
    try:
      newdata=ser.readline()
      if (str(newdata[0:6]) == "b'$GPRMC'"):
        print("===========================")
        latoriginal = str(newdata).split(",")
        tim=latoriginal[1].split(".")[0]
        lat="0"+latoriginal[3].split(".")[0]+latoriginal[3].split(".")[1]
        lng=latoriginal[5].split(".")[0]+latoriginal[5].split(".")[1]
        gps = "Latitude=" + str(lat) + " and Longitude=" + str(lng)
        lorasender = obj+sender+tim+lat+lng
        print(lorasender)
        print("===========================")
        except_counter = 0
        return lorasender
        break

    except serial.serialutil.SerialException:
      except_counter +=1
      print(except_counter)
      return False
  #########GET GPS#########

#Data collect Module
def sendDB(objid,cipher_text,merged):
  #DB Send config
  dhost = "192.168.31.200" #serverip
  ddb = "lora_local"
  duser = "localuser"
  dpass = "qweasdzxc"
  print("sendDB")
  try:
    conn = mysql.connector.connect(
      host = dhost,
      database = ddb,
      user = duser,
      password = dpass
    )

    print(objid)
    print(merged)
    #DB INSERT SQL
    sql = "INSERT INTO send (id,send,send_crypted) VALUES ("+str(objid)+",'"+str(merged)+"','"+str(cipher_text)+"')"
    print(sql)
    cursor = conn.cursor()
    cursor.execute(sql)
    conn.commit()
    print("insert success")
    #DB update SQL
    sql = "update predict set RPIcheck='1' where id= "+str(objid)
    print(sql)
    cursor = conn.cursor()
    cursor.execute(sql)
    conn.commit()
    print("update success")

    cursor.close()
    conn.close()
    if(result==[]):
      return 0
    else:
      return result
  except Error as e:
    print("DB connect Error")

#Crypted Module
def txt_shift(txt, shift):
  result = ""
  hexset=["0","1","2","3","4","5","6","7","8","9","a","b","c","d","e","f"]
  locate = ""
  for idi in range(0, len(txt)):
      for idj in range(0, len(hexset)):
          if(str(txt[idi]) == hexset[idj]):
              locate = idj
              break
      locate = locate+shift
      locate = locate%16
      result = result + str(hexset[locate])
  return result
#Crypted Module
def caesar_encryption(txt, shift):
  return txt_shift(txt, shift)
#Crypted Module
def caesar_decryption(txt, shift):
  return txt_shift(txt, -1 * shift)

shift_amount = 8

def checklength(objid):
  a = int(objid)
  b = len(str(a))
  if(b%2)==0:
    return objid
  else:
    objid = str(0)+str(objid)
    return objid

print("initial")
while True:
  #########Get Server Data#########
  #get new Identify data from Server module
  result = searchDB()
  time.sleep(1)
  if(result):
    objid = result[0][0]
    obj = result[0][2]
    #get data from gps module
    original = data_get(obj)
    #check length is even
    originalid = checklength(objid)
    merged = originalid+original
    if(merged):
      print(f"原始明文: {merged}")
      cipher_text = caesar_encryption(merged, shift_amount)
      print(f"加密密文: {cipher_text}")
      decryption_cipher_txt = caesar_decryption(cipher_text, shift_amount)
      print(f"解密結果: {decryption_cipher_txt}")
      sendDB(objid,cipher_text,merged)
    else:
      print("GPS merged failed")
  else:
    print("no result")

	#########Get Server Data#########



######### MQTT Crawler Settings  #########
shift_amount = 8
ACK = "NBIOTACK"
SYN = ""
ACK_SYN_data=""
status=0
IMEI = ""
data = ""
#status = 0(尚未收到SYN) / 1(收到SYN，送出SYN+ACK) / 2(收到ACK回傳，確認訊息已收到，準備上傳DB)
######### MQTT Crawler Settings  #########  
import pymysql
import logging
import json
import paho.mqtt.client as mqtt
import datetime as datetimeLibrary
import time
import mysql.connector
from mysql.connector import Error
import datetime




######### DB module  #########
def connectDb(dbName):
    try:
        mysqldb = pymysql.connect(
                host="140.128.101.127",
                user="lora",
                passwd="lora",
                database=dbName)
        print('Connected')
        return mysqldb
    except Exception as e:
        logging.error('Fail to connection mysql {}'.format(str(e)))
    return None


conn = connectDb('lora_server')
if conn is not None:
    cur = conn.cursor()    
def insertDB(checkType,time,temp,humidity,gps):
    sql = "INSERT INTO result (type, time, temperature, humidity, gps) VALUES (%s, %s, %s, %s, %s)"
    val = (checkType, time,temp ,humidity, gps)
    cur.execute(sql, val)
    conn.commit()
    print(cur.rowcount, "record inserted.")
    print(sql, val)

def selectDB():
    sql = 'select * from result'
    if cur is not None:
        cur.execute(sql)
        resultall = cur.fetchall()
        #print(resultall)
        print ('Fetch all rows:')
        for row in resultall:
            print (row) #數字代表column
######### DB module  #########

######### crypt module  #########
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

def caesar_encryption(txt, shift):
    return txt_shift(txt, shift)

def caesar_decryption(txt, shift):
    return txt_shift(txt, -1 * shift)
######### crypt module  #########

######### Three-way Handshake module  #########
def three_way_status(receive_data=""):
    #status = 0(尚未收到SYN) / 1(收到SYN，送出SYN+ACK) / 2(收到ACK回傳，確認訊息已收到，準備上傳DB)
    global status
    global ACK_SYN_data
    global ACK
    if(receive_data==""):
        print("no receive_data")
    if(status==0):
        SYN = receive_data
        ACK_SYN_data = ACK + SYN
        #(收到SYN，送出SYN+ACK)
        status = 1
        print("收到SYN，送出SYN+ACK")
        return ACK_SYN_data
    elif(status==1):
        ACK_received = receive_data
        ACK_length = len(ACK)
        if(ACK==ACK_received[0:ACK_length]):
            status = 2
            #收到ACK回傳，確認訊息已收到，準備上傳DB
            print("收到ACK回傳，確認訊息已收到，正在準備上傳DB")
            return True
        else:
            SYN = receive_data
            ACK_SYN_data = ACK + SYN
            #(收到SYN，送出SYN+ACK)
            print("仍收到SYN，繼續送出SYN+ACK")
            return False
    elif(status==2):
        print("收到ACK回傳，確認訊息已收到，正在準備上傳DB")
        return True
    
    else:
        print("error: "+status)
        print("receive_data: "+receive_data)

######### Three-way Handshake module  #########
######### RX send module  #########
def RX_sender(gwid,macAddr,ACK_SYN_data):
    #payload = '[{"macAddr":"'+macAddr+'","data":"'+ACK_SYN_data+'","gwid":"'+gwid+'","extra":{"port":2,"txpara":"2"}}]'
    payload = [{'macAddr':macAddr,'data':ACK_SYN_data,'gwid':gwid,'extra':{'port':2,'txpara':'2'}}]
    topic = "GIOT-GW/DL/"+gwid
    print (json.dumps(payload))
    print("topic = "+topic)
    try:
        client.publish( topic, json.dumps(payload))
        return True
    except:
        print("RX_sender publish error")
        return False

######### RX send module  #########



######### MQTT Crawler module  #########

# 當地端程式連線伺服器得到回應時，要做的動作
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    # 將訂閱主題寫在on_connet中
    # 如果我們失去連線或重新連線時
    # 地端程式將會重新訂閱
    client.subscribe("Forensics/Sensor")

# 當接收到從伺服器發送的訊息時要進行的動作
def on_message(client, userdata, msg):
    global msg_syn
    global msg_ack_syn
    global msg_ack
    global msg_ack_flag
    global msg_ack_validate

    global status
    global checkType
    global time
    global temp
    global humidity
    global gps
    global IMEI
    global data

	# print(msg.topic+" "+msg.payload.decode('utf-8'))   # 轉換編碼
    #utf-8才看得懂中文
    #print(msg.topic+" "+str(msg.payload))
    #print(msg.payload)
    #print(len(str(msg.payload)))
    if(len(str(msg.payload))<6):
        print("initial")
    else:
        #json module
        try:
            d = json.loads(msg.payload)[0]
            IMEI = d['IMEI']
            data = d['data']
            temp = d['temperature']
            humidity = d['humidity']
            ack = d['ACK']
            print("IMEI = "+IMEI)
            print("data = "+data)
        except:
            d = json.loads(msg.payload)[0]
            print(d)
        
        if(IMEI =="862177043502262"):
            print("========================Get Correct IMEI=======================================")

            now = datetime.datetime.now()
            print ("Current date and time : "+now.strftime("%Y-%m-%d %H:%M:%S"))
            print("IMEI="+str(IMEI)+",data="+str(data)+",temperature="+str(temp)+",humidity="+str(humidity))
            print("===start rx===")
            if(data==""):
                print("----Initial SYN----")
                msg_syn = data
                try:
                    msg_ack_syn = msg_syn[0:5]+msg_ack
                    print ("0,SYN+ACK，msg_ack_syn = "+msg_ack_syn)
                    client.publish( "Forensics/PC", msg_ack_syn)
                except:
                    print("Error in status 0 sending rx")
            elif(msg_syn != data):
                print("----Get New SYN----")
                msg_syn = data
                try:
                    msg_ack_syn = msg_syn[0:5]+msg_ack
                    print ("3,送出新SYN+ACK，msg_ack_syn = "+msg_ack_syn)
                    client.publish( "Forensics/PC", msg_ack_syn)
                except:
                    print("Error in status 0 sending rx")
            else:
                print("----Get Old SYN，判斷是否有ACK----")
                print("ack = "+str(ack))
                print("msg_ack = "+str(msg_ack))
                if(ack == msg_ack):
                    print("2,已確認接收，開始上傳Data")
                    original_data = caesar_decryption(msg_syn,8)
                    checkType=original_data[2:4]
                    temp=original_data[4:8]
                    humidity=original_data[8:12]
                    time=original_data[12:22]
                    gps=original_data[22:42]

                    print(msg_ack)
                    print("checkType = "+checkType)
                    print("temp = "+temp)
                    print("humidity = "+humidity)
                    print("time = "+time)
                    print("gps = "+gps)
                    if(insertDB(checkType,time,temp,humidity,gps)):
                        print("Insert into DB")
                    else:
                        print("Error to insert into DB")
                else:
                    print("1,尚未接收，繼續送出SYN+ACK")
                    try:
                        msg_ack_syn = data[0:5]+msg_ack
                        payload = ""
                        print ("msg_ack_syn = "+msg_ack_syn)
                        client.publish( "Forensics/PC", msg_ack_syn)
                    except:
                        print("Error in status 0 sending rx")
        print("===============================================================")
msg_syn = ""
msg_ack_syn = ""
msg_ack = "NBIOTACK"

checkType=""
time=""
temp=""
humidity=""
gps=""
# 連線設定
client = mqtt.Client()            # 初始化地端程式 
client.on_connect = on_connect    # 設定連線的動作
client.on_message = on_message    # 設定接收訊息的動作

client.username_pw_set("course","iot999")      # 設定登入帳號密碼 
client.connect("140.128.99.71", 1883, 60) # 設定連線資訊(IP, Port, 連線時間)

# 開始連線，執行設定的動作和處理重新連線問題
# 也可以手動使用其他loop函式來進行連接
client.loop_forever()

######### MQTT Crawler module  #########
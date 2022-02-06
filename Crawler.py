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
    print(sql, val)
    try:
        cur.execute(sql, val)
        conn.commit()
        print(cur.rowcount, "record inserted.")
        return True
    except:
        print("Insert DB Error")
        return False

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

######### MQTT Crawler module  #########

# 當地端程式連線伺服器得到回應時，要做的動作
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    # 將訂閱主題寫在on_connet中
    # 如果我們失去連線或重新連線時
    # 地端程式將會重新訂閱
    client.subscribe("GIOT-GW/UL/1C497B455949")
    client.subscribe("GIOT-GW/UL/1C497B455A27")
    client.subscribe("GIOT-GW/UL/1C497BFA3280")
    client.subscribe("GIOT-GW/UL/80029C5A3306")
    client.subscribe("GIOT-GW/UL/80029C5A33A5")

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
	# print(msg.topic+" "+msg.payload.decode('utf-8'))   # 轉換編碼
    #utf-8才看得懂中文
    #print(msg.topic+" "+str(msg.payload))
    #print(msg.payload)
    #print(len(str(msg.payload)))
    if(len(str(msg.payload))<6):
        print("initial")
    else:
        #json module
        d = json.loads(msg.payload)[0]
        gwid = d['gwid']
        macAddr = d['macAddr']
        receive_data = d['data']
        data_length = len(receive_data)


        if(macAddr =="0000000051000002" or macAddr =="0000000051000000" or macAddr =="0000000051000001" or macAddr =="0000000051000003"):
            print("===============================================================")
            if(receive_data==""):
                print("===No data no rx===")
            else:
                print("狀態 : "+str(status))
                # 取得我們對應之卡號，印出基本訊息
                print(msg.topic)
                now = datetime.datetime.now()
                print ("Current date and time : "+now.strftime("%Y-%m-%d %H:%M:%S"))
                print("receive_data = "+receive_data)
                print("gwid="+gwid+",macAddr="+macAddr+",receive_data="+receive_data)
                print(d)
                print("===start rx===")	  
                if(status==0):
                    try:
                        msg_syn = receive_data
                        msg_ack_syn = msg_ack+msg_syn
                        RX_sender(gwid,macAddr,msg_ack_syn)
                        print("收到SYN，送出SYN+ACK")
                        status = 1
                    except:
                        print("Error in status 0 sending rx")
                elif(status==1):
                    if(msg_syn == receive_data):
                        try:
                            msg_syn = receive_data
                            msg_ack_syn = msg_ack+msg_syn
                            RX_sender(gwid,macAddr,msg_ack_syn)
                            print("仍收到SYN，再次送出ACK+SYN")
                            #06 a1 2600 8625 1640526506 0241097760 1203539975
                            original_data = caesar_decryption(msg_syn,8)
                            checkType=original_data[2:4]
                            temp=original_data[4:8]
                            humidity=original_data[8:12]
                            time=original_data[12:22]
                            gps=original_data[22:42]
                            print("checkType = "+checkType)
                            print("temp = "+temp)
                            print("humidity = "+humidity)
                            print("time = "+time)
                            print("gps = "+gps)
                            status = 1
                        except:
                            print("Error in status 1 sending rx")
                    elif(msg_ack == receive_data):
                        RX_sender(gwid,macAddr,msg_ack)
                        print("收到ACK，等待上傳")
                        status = 2
                        if(status==2):
                            #06 a1 2600 8625 1640526506 0241097760 1203539975
                            original_data = caesar_decryption(msg_syn,8)
                            checkType=original_data[2:4]
                            temp=original_data[4:8]
                            humidity=original_data[8:12]
                            time=original_data[12:22]
                            gps=original_data[22:42]
                            print("checkType = "+checkType)
                            print("temp = "+temp)
                            print("humidity = "+humidity)
                            print("time = "+time)
                            print("gps = "+gps)

                            if(insertDB(checkType,time,temp,humidity,gps)):
                                print("Insert into DB")
                                status = 0
                            else:
                                print("Error to insert into DB")

                        else:
                            print("Error in status 1 recieving correct rx")
                            print("RX = "+str(receive_data))

            print("===End rx===")	
            print("===============================================================")
msg_syn = ""
msg_ack_syn = ""
msg_ack = "1a2b"
msg_ack_flag = "1a"
msg_ack_validate = ""

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
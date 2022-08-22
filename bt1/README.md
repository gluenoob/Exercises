# Bài tập 1: Mô phỏng RRC Connection Establishment
![](image/bt1.png) <br />
Điều kiện:
- 10 UE, mỗi UE gửi 10 RRCSetupRequest/s
- Sau 200s tính ra RRC_SR

### Các structure cho từng msg
1. RRCSetupRequest: ![](image/msg1.png)
2. RRCSetup: ![](image/msg2.png)
3. RRCSetupComplete: ![](image/msg3.png)

### UE và gNB đơn
Dùng giao thức TCP để kết nối: <br />

- header file: bt1.h
- UE: ue_single.c
- gNB: gNB.c

### Multi-UE gNB
Dùng non-blocking IO để gNB xử lý nhiều UE.<br />
Dùng multi-threading để 10 UE gửi và nhận đồng thời. <br />

- header file: bt1.h
- UE: ue_poll1.c, ue_poll_ueid (thêm ueid cho msg3)
- gNB: gnb_poll1.c, gnb_poll_ueid (thêm RRC_ReAtt và ue_list)

# Lab 6 執行結果範例

## Task 1

```
./waf --run linear-move > your_result
```
- 如果 linear-move 有寫對，"your_result" 應該會與 "task1_result" 相同 (第4行可以不同)

## Task 2

```
./waf --run "random-walk --fileName=movement_for_debug" 2>your_stderr_result
```
- 如果 random-walk 有寫對，"your_stderr_result" 應該會與 "task2_result" 完全相同

```
./get_signal_power.sh <pcap file> <AP's MAC addr> 
```
- 你的 pcap file 產生的訊號強度種類應該要比 discrete.pcap 所產生的還多

## Task 3

```
python3 parser.py parser_input.pcap > your_result
```
- 如果 parser 有寫對，"your_result" 應該會與 "task3_result" 相同
- 如果使用 "Disassociation message" 做為判斷 "connection duration" 的依據，則該項可能會有些差距

UCLでのLabViewのコード

基本的にLabView24Q1で作製

Arduinoフォルダは過去のArduinoのコードを収容

DAQフォルダはLabViewで触覚センサアンプ、Arduinoと通信して、データを表示・記録するVIを格納
1.DAQ-Access Version: USB-6009などのDAQの0−1chにセンサのアナログ出力を接続し、DAQから電源も供給し、Arduino無しで稼働するバージョン
1.Serial-Acceess Version: PCのUSBポートにArduinoのSerialから出力して接続するバージョン
  
原則としていずれもLabViewが必要だが、CommunityバージョンでOK

Serial版は、WindowsのApp版も置いてある．（Serial-Access Version->App->Application.exe）

#include <Miner.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <utils.h>
#include <mbedtls/md.h>
#include <config.h>

void Miner::setup(String name, String poolUrl, int poolPort, String walletAddress) {
  _poolPort = poolPort;
  _walletAddress = walletAddress;
  _name = name;

  int err = WiFi.hostByName((const char *)poolUrl.c_str(), _poolIp);

  if (err = 1)
  {
    _poolExist = true;
    Serial.print("Pool Ip: ");
    Serial.println(_poolIp);
  }
  else
  {
    _poolExist = false;
    Serial.print("Error Code: ");
    Serial.println(err);
  }
  
}

void Miner::run(void) {
  if (!_poolExist){
   return; 
  }

  // Connect to pool
  WiFiClient client;

  if (!client.connect(_poolIp, _poolPort)) {
    Serial.println("Fallo la conexion con la pool");
    return;
  }

  String payload = String("{\"id\": 1, \"method\": \"mining.subscribe\", \"params\": []}\n");

  client.print(payload.c_str());
  String line = client.readStringUntil('\n');

  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, line);

  if (error) {
    Serial.print("Fallo al parsear el json: ");
    Serial.println(error.c_str());
    return;
  }

  JsonArray result = doc["result"];
  String subDetails = String((const char *)result[0][0][1]);
  String extraoncel1 = String((const char *)result[1]);
  int extraoncel1Size = result[2];

  line = client.readStringUntil('\n');
  error = deserializeJson(doc, line);

  if (error) {
    Serial.print("Fallo al parsear el json: ");
    Serial.println(error.c_str());
    return;
  }

  String method = String((const char *)doc["method"]);

  // Mostramos por puerto serial
  Serial.print("Sub details: ");
  Serial.println(subDetails);
  Serial.print("Extraoncel 1: ");
  Serial.println(extraoncel1);
  Serial.print("Extraoncel 1 size: ");
  Serial.println(extraoncel1Size);
  Serial.print("Method: ");
  Serial.println(method);

  // Pool: authorize work
  payload = String("{\"id\": 2, \"method\": \"mining.authorize\", "
                         "\"params\": [\"") +
                  _walletAddress + String("\", \"password\"]}\n");
  client.print(payload.c_str());

  line = client.readStringUntil('\n');
  error = deserializeJson(doc, line);

  if (error) {
    Serial.print("Fallo al parsear el json: ");
    Serial.println(error.c_str());
    return;
  } else if (doc["result"] == false) {
    Serial.println("Error al authorizar el minero");
    return;
  }

  JsonArray params = doc["params"];
  String jobId = String((const char *)params[0]);
  String prevHash = String((const char *)params[1]);
  String coinB1 = String((const char *)params[2]);
  String coinB2 = String((const char *)params[3]);
  JsonArray merkleBranch = params[4];
  String version = String((const char *)params[5]);
  String nBits = String((const char *)params[6]);
  String nTime = String((const char *)params[7]);
  bool cleanJobs = params[8];

  Serial.print("Job Id: ");
  Serial.println(jobId);
  Serial.print("Prev Hash: ");
  Serial.println(prevHash);
  Serial.print("Coinbase 1: ");
  Serial.println(coinB1);
  Serial.print("Coinbase 2: ");
  Serial.println(coinB2);
  Serial.print("Merkle Branch Size: ");
  Serial.println(merkleBranch.size());
  Serial.print("Version: ");
  Serial.println(version);
  Serial.print("N Bits: ");
  Serial.println(nBits);
  Serial.print("N Time: ");
  Serial.println(nTime);
  Serial.print("Clean Jobs: ");
  Serial.println(cleanJobs);

  // leemos las siguientes dos lineas
  client.readStringUntil('\n');
  client.readStringUntil('\n');

  // Calculamos el target
  String target = nBits.substring(2);
  int ceros = (int)strtol(nBits.substring(0, 2).c_str(), 0, 16) - 3;

  for (int k = 0; k < ceros; k++)
  {
    target += String("00");
  }
  
  for (int k = 0; k < 64 - target.length(); k++)
  {
    target = String("0") + target;
  }

  Serial.print("Target: ");
  Serial.println(target);

  // Byte_array target
  size_t sizeTarget = toByteArray(target.c_str(), 32, _byteArrayTarget);
  uint8_t buff;

  for (size_t i = 0; i < 16; i++)
  {
    buff = _byteArrayTarget[i];
    _byteArrayTarget[i] = _byteArrayTarget[sizeTarget - i - 1];
    _byteArrayTarget[sizeTarget - i - 1] = buff;
  }
  
  // Get extraoncel 2
  uint32_t extraoncel2ABin = esp_random();
  uint32_t extraoncel2BBin = esp_random();

  String extraoncel2A = String(extraoncel2ABin, HEX);
  String extraoncel2B = String(extraoncel2BBin, HEX);

  for (int k = 0; k < 8 - extraoncel2A.length(); k++)
  {
    extraoncel2A = String("0") + extraoncel2A;
  }

  for (int k = 0; k < 8 - extraoncel2B.length(); k++)
  {
    extraoncel2B = String("0") + extraoncel2B;
  }
  
  String extraoncel2 = extraoncel2A + extraoncel2B;

  Serial.print("Extraoncel 2: ");
  Serial.println(extraoncel2);

  // Get coinbase
  String coinBase = coinB1 + extraoncel1 + extraoncel2 + coinB2;
  Serial.print("Coinbase: ");
  Serial.println(coinBase);

  size_t strLength = coinBase.length() / 2;
  uint8_t byteArray[strLength];
  size_t res = toByteArray(coinBase.c_str(), strLength * 2, byteArray);

  Serial.print("Coinbase bytes size: ");
  Serial.println(res);

  Serial.print("Coinbase bytes: ");

  for (size_t i = 0; i < res; i++)
  {
    Serial.printf("%02x ", byteArray[i]);
  }

  Serial.println();
  
  // Creating the hash (SHA256)
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);

  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, byteArray, strLength);
  mbedtls_md_finish(&ctx, _interResult);
  
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, _interResult, 32);
  mbedtls_md_finish(&ctx, _shaResult);

  Serial.print("Coinbase hash: ");

  for (size_t i = 0; i < 32; i++)
  {
    Serial.printf("%02x ", _shaResult[i]);
  }

  Serial.println();

  // Copy coinbase hash
  byte merkleResult[32];
  
  for (size_t i = 0; i < 32; i++)
  {
    merkleResult[i] = _shaResult[i];
  }
  
  byte merkeleConcated[64];

  for (size_t k = 0; k < merkleBranch.size(); k++) /*** Optimizar */
  {
    const char* merkeleElement = (const char*) merkleBranch[k];
    uint8_t byteArray[32];
    size_t res = toByteArray(merkeleElement, 32, byteArray);

    for (size_t i = 0; i < 32; i++)
    {
      merkeleConcated[i] = merkleResult[i];
      merkeleConcated[i + 32] = byteArray[i];
    }

    Serial.println("Merkele element " + k);
    Serial.print("Merkele concated: ");

    for (size_t i = 0; i < 64; i++)
    {
      Serial.print(merkeleConcated[i]);
    }
    
    Serial.println();

    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, merkeleConcated, 64);
    mbedtls_md_finish(&ctx, _interResult);

    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, _interResult, 32);
    mbedtls_md_finish(&ctx, merkleResult);

    Serial.print("Merkle sha: ");

    for (size_t i = 0; i < 32; i++)
    {
      Serial.print(merkleResult[i]);
    }
    Serial.println();
  }
  
  // Merkele root from merkele result
  String merkleRoot = "";

  for (size_t i = 0; i < 32; i++)
  {
    merkleRoot += String(merkleResult[i], HEX);
  }

  // Calculate block header
  String blockHeaderStr = version + prevHash + merkleRoot + nBits + nTime;
  size_t strLen = blockHeaderStr.length() / 2;
  uint8_t byteArrayBlockHeader[strLen];

  res = toByteArray(blockHeaderStr.c_str(), strLen * 2, byteArrayBlockHeader);

  Serial.printf("Block header bytes (%02x): ", strLen);
  Serial.println(res);

  // Reverse Array
  reverseArray(byteArrayBlockHeader, 0, 4);
  reverseArray(byteArrayBlockHeader, 36, 32);
  reverseArray(byteArrayBlockHeader, 72, 4);

  // Show data
  showData("Version: ", byteArrayBlockHeader, 0, 4);
  showData("Prev Hash: ", byteArrayBlockHeader, 4, 36);
  showData("Merkele Root: ", byteArrayBlockHeader, 36, 68);
  showData("Time: ", byteArrayBlockHeader, 68, 72);
  showData("Difficulty: ", byteArrayBlockHeader, 72, 76);
  showData("Nonce: ", byteArrayBlockHeader, 76, 80);

  // Search a valid nonce
  uint32_t nonce = 0;
  bool mining = true;

  while (mining)
  {
    uint64_t startMining = esp_timer_get_time();
    byteArrayBlockHeader[76] = (nonce >> 0) & 0xFF;
    byteArrayBlockHeader[77] = (nonce >> 8) & 0xFF;
    byteArrayBlockHeader[78] = (nonce >> 16) & 0xFF;
    byteArrayBlockHeader[79] = (nonce >> 24) & 0xFF;

    // Generate hash
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, byteArrayBlockHeader, 80);
    mbedtls_md_finish(&ctx, _interResult);

    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, _interResult, 32);
    mbedtls_md_finish(&ctx, _shaResult);

    // Check if valid header
    if (checkValid(_shaResult, _byteArrayTarget))
    {
      _validShares++;
      Serial.printf("%s on core %d\n", _name.c_str(), xPortGetCoreID());
      Serial.printf("Valid completed mining: %d | 0x%x\n", nonce, nonce);

      // Mining
      payload = String("{\"params\": [\"") + _walletAddress
                + String("\", \"") + jobId
                + String("\", \"") + extraoncel2
                + String("\", \"") + nTime
                + String("\", \"") + nonce
                + String("\"], \"id\": 1, \"method\": \"mining.submit\"");
      client.print(payload.c_str());

      line = client.readStringUntil('\n');
      Serial.print("Reciving: ");
      Serial.println(line);

      // Stop Mining
      mining = false;
    }
    else
    {
      _invalidShares++;
    }
    
    nonce++;

    if (nonce >= MAX_NONCE)
    {
      Serial.printf("%s on core %d: Nonce > MAX_NONCE\n", _name.c_str(), xPortGetCoreID());
      mining = false;
    }

    _hashrate = 1000000 / (esp_timer_get_time() - startMining);
  }
  
  // Exit mining
  mbedtls_md_free(&ctx);

  // Close connection pool
  client.stop();
}

long unsigned int Miner::getData(meassurment type)
{
  switch (type)
  {
    case VALID_SHARES:
      return _validShares;
    case INVALID_SHARES:
      return _invalidShares;
    case HASHRATE:
      return _hashrate;
    default:
      return _validShares;
    }
}
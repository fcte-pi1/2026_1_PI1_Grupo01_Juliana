# Testes de Armazenamento Local (NVS & SPIFFS) - Micromouse

Este subdiretório contém os códigos para os testes de armazenamento local no microcontrolador ESP32 do robô Micromouse. O objetivo é demonstrar que o hardware é capaz de armazenar dados de configuração persistentes (via **NVS**) e leituras/telemetria de sensores (via **SPIFFS**).

---

## 📂 Estrutura do Diretório

* **`main/storage_test_main.c`**: Código-fonte em C (ESP-IDF) para rodar na placa física ESP32.
* **`simulate_storage.py`**: Script em Python que simula o comportamento exato de leitura e escrita das memórias NVS e SPIFFS localmente no seu computador.
* **`CMakeLists.txt` e `main/CMakeLists.txt`**: Arquivos de configuração do sistema de compilação CMake.

---

## 🐍 1. Executando a Simulação em Python (Sem a ESP Física)

Como você está sem a ESP física agora, o script `simulate_storage.py` simula as gravações e leituras de arquivos que o firmware faria.

### Passo a passo para rodar:

1. Abra o terminal na pasta deste teste:
   ```bash
   cd src/firmware/storage_test
   ```

2. Execute o script:
   ```bash
   python simulate_storage.py
   ```

3. O console mostrará:
   * **Cenário 1 (NVS):** Gravando `corrida_id = 3` e relendo para validar se o valor permanece íntegro.
   * **Cenário 2 (SPIFFS):** Gravando um payload de telemetria simulado em formato JSON, lendo a linha, efetuando o parse e validando os dados.

4. Ao final, o script perguntará se deseja testar o envio opcional para o backend FastAPI local (`s/n`). Se você tiver o backend rodando e configurado conforme o README do backend, pode digitar `s` para testar a comunicação.

---

## 🔌 2. Rodando na Placa ESP32 (Firmware Real)

Quando você estiver com o hardware físico em mãos, poderá compilar o firmware real.

### Tabela de Partições Customizada (Importante para SPIFFS)
Para que o sistema de arquivos SPIFFS monte com sucesso no hardware da ESP32, configuramos uma partição personalizada na memória flash do chip.
* **`partitions.csv`**: Define a partição de dados do tipo `spiffs` com tamanho de 1 MB.
* **`sdkconfig.defaults`**: Garante que o processo de build do ESP-IDF ative automaticamente a opção de tabela de partições customizada (`CONFIG_PARTITION_TABLE_CUSTOM=y`) e a aponte para o nosso `partitions.csv`. Não é necessário configurar isso no `menuconfig` manualmente.

### Utilizando a Extensão do ESP-IDF no VS Code:
1. Abra o projeto no VS Code.
2. Certifique-se de que a extensão oficial do **ESP-IDF** da Espressif está instalada e configurada.
3. Clique no botão de seleção de target (barra inferior do VS Code) e escolha o chip correto (ex: `ESP32` ou `ESP32-S3`).
4. Clique em **Build** (ícone de engrenagem) para compilar o código. O ESP-IDF usará o `sdkconfig.defaults` para gerar a configuração de partições apropriada automaticamente.
5. Conecte a placa via USB, selecione a porta COM correspondente e clique em **Flash** (ícone de raio) para gravar no chip.
6. Clique em **Monitor** (ícone de monitor) para abrir o terminal serial (UART).

### Saída Esperada no Monitor Serial:
Você deverá ver logs claros no terminal semelhantes a este:
```text
I (312) STORAGE_TEST: ==================================================
I (312) STORAGE_TEST: Iniciando Testes de Armazenamento Local - Micromouse
I (322) STORAGE_TEST: ==================================================
I (332) STORAGE_TEST: === [INICIANDO TESTE NVS] ===
I (342) STORAGE_TEST: Gravando corrida_id = 3 na NVS...
I (382) STORAGE_TEST: [NVS TEST] Aprovado: Gravado=3, Lido=3
I (1392) STORAGE_TEST: === [INICIANDO TESTE SPIFFS] ===
I (1412) STORAGE_TEST: SPIFFS montado: Total: 956 KB, Usado: 1 KB
I (1412) STORAGE_TEST: Abrindo arquivo /spiffs/telemetry.json para gravação...
I (1422) STORAGE_TEST: Escrevendo leituras no arquivo: {"posicao_x":5,"posicao_y":7,"nivel_bateria":84.5,"velocidade":0.42}
I (1432) STORAGE_TEST: Arquivo fechado.
I (1432) STORAGE_TEST: Reabrindo arquivo /spiffs/telemetry.json para leitura...
I (1442) STORAGE_TEST: Dados lidos do SPIFFS: {"posicao_x":5,"posicao_y":7,"nivel_bateria":84.5,"velocidade":0.42}
I (1452) STORAGE_TEST: SPIFFS desmontado.
I (1452) STORAGE_TEST: [SPIFFS TEST] Aprovado: Leituras dos sensores gravadas e validadas com sucesso!
I (1462) STORAGE_TEST: ==================================================
I (1472) STORAGE_TEST: Resumo dos Testes de Armazenamento:
I (1472) STORAGE_TEST:   Teste NVS (Dados Básicos):      PASSOU
I (1482) STORAGE_TEST:   Teste SPIFFS (Dados Sensores): PASSOU
I (1492) STORAGE_TEST: ==================================================
```

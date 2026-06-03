import os
import json
import time
from datetime import datetime, timezone

# Caminhos para os arquivos que simulam a memória física do ESP32
MOCK_NVS_FILE = "mock_nvs.json"
MOCK_SPIFFS_FILE = "mock_spiffs.jsonl"


# ====================================================================
# Cenário 1: Simulação de NVS (Dados Básicos)
# ====================================================================
def test_mock_nvs():
    print("\n=== [INICIANDO TESTE NVS SIMULADA] ===")
    
    # 1. Dados básicos para salvar (configurações do sensor e wifi)
    dados_gravar = {
        "wifi_ssid": "Micromouse_Mesh",
        "calibracao_sensor": 512
    }
    
    print(f"[*] Gravando dados de configuração na NVS: {dados_gravar}")
    try:
        # Se houver corrida_id pré-existente (salvo pelo backend), preserve-o
        corrida_id_atual = None
        if os.path.exists(MOCK_NVS_FILE):
            try:
                with open(MOCK_NVS_FILE, "r") as f:
                    corrida_id_atual = json.load(f).get("corrida_id")
            except Exception:
                pass
        
        with open(MOCK_NVS_FILE, "w", encoding="utf-8") as f:
            payload_gravar = dados_gravar.copy()
            if corrida_id_atual:
                payload_gravar["corrida_id"] = corrida_id_atual
            json.dump(payload_gravar, f, indent=4)
        print("[+] Gravação na NVS realizada com sucesso.")
    except Exception as e:
        print(f"[-] Erro ao gravar na NVS: {e}")
        return False

    # Pequeno atraso para simular o tempo de processamento
    time.sleep(0.5)

    # 2. Leitura dos dados simulados
    print("[*] Reabrindo a NVS e efetuando leitura de 'calibracao_sensor'...")
    try:
        if not os.path.exists(MOCK_NVS_FILE):
            print("[-] Erro: Arquivo de NVS não encontrado!")
            return False
            
        with open(MOCK_NVS_FILE, "r", encoding="utf-8") as f:
            dados_lidos = json.load(f)
            
        valor_lido = dados_lidos.get("calibracao_sensor")
        print(f"[+] Valor lido da NVS: calibracao_sensor = {valor_lido}")
    except Exception as e:
        print(f"[-] Erro ao ler da NVS: {e}")
        return False

    # 3. Validação dos dados
    if valor_lido == dados_gravar["calibracao_sensor"]:
        print(f"[OK] [NVS TEST] Aprovado! Gravado={dados_gravar['calibracao_sensor']}, Lido={valor_lido}")
        return True
    else:
        print(f"[ERRO] [NVS TEST] Reprovado! Valores divergem. Gravado={dados_gravar['calibracao_sensor']}, Lido={valor_lido}")
        return False


# ====================================================================
# Cenário 2: Simulação de SPIFFS (Dados de Sensores)
# ====================================================================
def test_mock_spiffs():
    print("\n=== [INICIANDO TESTE SPIFFS SIMULADO] ===")
    
    # 1. Dados de sensor simulados
    telemetria_sensor = {
        "posicao_x": 5,
        "posicao_y": 7,
        "nivel_bateria": 84.5,
        "velocidade": 0.42,
        "timestamp": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z")
    }
    
    print(f"[*] Gravando dados de telemetria no SPIFFS (/spiffs/telemetry.json)...")
    try:
        # Gravando em formato JSON-Lines (um JSON por linha)
        with open(MOCK_SPIFFS_FILE, "w", encoding="utf-8") as f:
            f.write(json.dumps(telemetria_sensor) + "\n")
        print("[+] Gravação de arquivo no SPIFFS concluída.")
    except Exception as e:
        print(f"[-] Erro ao gravar no SPIFFS: {e}")
        return False

    time.sleep(0.5)

    # 2. Leitura dos dados simulados
    print("[*] Reabrindo arquivo do SPIFFS e lendo dados dos sensores...")
    try:
        if not os.path.exists(MOCK_SPIFFS_FILE):
            print("[-] Erro: Arquivo de SPIFFS não encontrado!")
            return False
            
        with open(MOCK_SPIFFS_FILE, "r", encoding="utf-8") as f:
            linhas = f.readlines()
            
        if not linhas:
            print("[-] Erro: Arquivo de SPIFFS está vazio!")
            return False
            
        dados_lidos = json.loads(linhas[0].strip())
        print(f"[+] Dados recuperados do SPIFFS: {dados_lidos}")
    except Exception as e:
        print(f"[-] Erro ao ler do SPIFFS: {e}")
        return False

    # 3. Validação dos dados de sensores recuperados
    validation_pass = True
    for campo in ["posicao_x", "posicao_y", "nivel_bateria", "velocidade"]:
        if dados_lidos.get(campo) != telemetria_sensor[campo]:
            print(f"[-] Divergência no campo '{campo}': Gravado={telemetria_sensor[campo]}, Lido={dados_lidos.get(campo)}")
            validation_pass = False

    if validation_pass:
        print("[OK] [SPIFFS TEST] Aprovado! Telemetria gravada e validada localmente com sucesso.")
        return True
    else:
        print("[ERRO] [SPIFFS TEST] Reprovado! Houve inconsistência nos dados de sensores lidos do SPIFFS.")
        return False


# ====================================================================
# Cenário 3 (Opcional): Integração de Envio com o Backend FastAPI
# ====================================================================
def sync_with_backend():
    print("\n=== [OPCIONAL: SINCRONIZAÇÃO COM BACKEND FASTAPI] ===")
    print("[*] Tentando se conectar ao servidor local na porta 8000...")
    
    try:
        import requests
    except ImportError:
        print("[-] Biblioteca 'requests' não encontrada. Instale com 'pip install requests' para rodar este teste.")
        return

    url = "http://localhost:8000/telemetria"
    
    # 1. Ler o corrida_id salvo na NVS simulada (se existir)
    corrida_id = None
    if os.path.exists(MOCK_NVS_FILE):
        try:
            with open(MOCK_NVS_FILE, "r") as f:
                nvs = json.load(f)
                corrida_id = nvs.get("corrida_id")
        except Exception:
            pass

    # 2. Ler telemetria salva no SPIFFS simulado
    if not os.path.exists(MOCK_SPIFFS_FILE):
        print("[-] Nenhum dado de telemetria encontrado no SPIFFS simulado. Execute o teste do SPIFFS primeiro.")
        return
        
    try:
        with open(MOCK_SPIFFS_FILE, "r") as f:
            linhas = f.readlines()
        if not linhas:
            print("[-] O arquivo de SPIFFS está vazio.")
            return
        dados_telemetria = json.loads(linhas[0].strip())
    except Exception as e:
        print(f"[-] Erro ao carregar telemetria do SPIFFS: {e}")
        return

    # Ajusta o payload de acordo com a regra de corrida do backend
    # Se não temos corrida_id salvo, iniciamos enviando labirinto_id = 1 para criar uma nova corrida
    payload = {
        "timestamp": dados_telemetria["timestamp"],
        "posicao_x": dados_telemetria["posicao_x"],
        "posicao_y": dados_telemetria["posicao_y"],
        "nivel_bateria": dados_telemetria["nivel_bateria"],
        "velocidade": dados_telemetria["velocidade"]
    }
    
    if corrida_id:
        payload["corrida_id"] = corrida_id
        print(f"[*] Enviando telemetria com corrida_id = {corrida_id} (salvo em NVS)...")
    else:
        payload["labirinto_id"] = 1  # 4x4 (padrão)
        print("[*] NVS vazia ou sem corrida_id. Iniciando nova corrida enviando labirinto_id = 1...")

    # 3. Fazer requisição POST
    try:
        response = requests.post(url, json=payload, timeout=3)
        if response.status_code in [200, 201]:
            retorno = response.json()
            nova_corrida_id = retorno.get("corrida_id")
            print(f"[OK] Envio bem-sucedido! Resposta do servidor: {retorno}")
            
            # Se for uma nova corrida criada pelo servidor, atualizar o corrida_id na NVS simulada
            if not corrida_id and nova_corrida_id:
                print(f"[+] Salvando nova corrida_id = {nova_corrida_id} na NVS para pacotes subsequentes...")
                try:
                    nvs_dados = {}
                    if os.path.exists(MOCK_NVS_FILE):
                        with open(MOCK_NVS_FILE, "r") as f:
                            nvs_dados = json.load(f)
                    nvs_dados["corrida_id"] = nova_corrida_id
                    with open(MOCK_NVS_FILE, "w") as f:
                        json.dump(nvs_dados, f, indent=4)
                except Exception as e:
                    print(f"[-] Falha ao atualizar NVS: {e}")
        else:
            print(f"[ERRO] Falha no envio. Servidor respondeu com código {response.status_code}: {response.text}")
    except requests.exceptions.RequestException as e:
        print(f"[ERRO] Não foi possível conectar ao servidor backend: {e}")
        print("DICA: Certifique-se de que o backend FastAPI está rodando localmente (http://localhost:8000)")


# ====================================================================
# Ponto de Entrada da Simulação
# ====================================================================
if __name__ == "__main__":
    print("==================================================")
    print("Simulador de Armazenamento Local ESP32 (Micromouse)")
    print("==================================================")
    
    # Executa os testes locais de armazenamento independente do backend
    nvs_ok = test_mock_nvs()
    spiffs_ok = test_mock_spiffs()
    
    print("\n==================================================")
    print("RESUMO DOS TESTES SIMULADOS:")
    print(f"  NVS (Dados Básicos):      {'PASSOU' if nvs_ok else 'FALHOU'}")
    print(f"  SPIFFS (Dados Sensores): {'PASSOU' if spiffs_ok else 'FALHOU'}")
    print("==================================================")

    # Pergunta se deseja tentar integração com o backend
    opcao = input("\nDeseja testar envio da telemetria armazenada ao backend FastAPI? (s/n): ").strip().lower()
    if opcao == 's':
        sync_with_backend()

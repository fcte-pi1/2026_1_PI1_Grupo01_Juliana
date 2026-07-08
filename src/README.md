## 🚀 Como executar o software

> **Pré-requisitos gerais:** Git, Python 3.10+, Node.js 18+, ESP-IDF v6.x

---

### 💻 Backend (FastAPI)

```bash
cd src/backend

# Criar e ativar ambiente virtual
python -m venv venv
source venv/bin/activate        # Windows: venv\Scripts\activate

# Instalar dependências
pip install -r requirements.txt

# Aplicar migrações e popular o banco com os labirintos (4x4, 8x8 e 16x16)
PYTHONPATH=. alembic upgrade head
PYTHONPATH=. python seed.py

# Subir o servidor
uvicorn app.main:app --reload --host 0.0.0.0
```

A API estará disponível em `http://localhost:8000`.  
Documentação interativa: `http://localhost:8000/docs`

> ⚠️ Use `--host 0.0.0.0` para que o ESP32 consiga acessar o backend pela rede local.

---

### 🌐 Frontend (React + Vite)

Em outro terminal:

```bash
cd src/frontend

npm install
npm run dev
```

Acesse `http://localhost:5173` no navegador.

> O frontend já assume o backend em `localhost:8000`. Nenhuma configuração adicional é necessária para rodar tudo na mesma máquina.

---

### 🤖 Firmware (ESP32 / ESP-IDF)

#### Pré-requisitos
- ESP-IDF v6.x instalado ([guia oficial](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/))
- ESP32 conectado via USB

#### Configurar rede e backend

Edite o topo de `src/firmware/odometria/components/telemetria/telemetria.c`:

```c
#define WIFI_SSID       "nome_da_sua_rede"
#define WIFI_PASS       "senha_da_rede"
#define BACKEND_WS_URI  "ws://IP_DO_PC:8000/ws/telemetria"
```

Para descobrir o IP do seu PC na rede:
- **macOS:** `ipconfig getifaddr en0`
- **Linux:** `hostname -I`
- **Windows:** `ipconfig`

> 💡 Use o hotspot do celular como rede — redes institucionais costumam bloquear comunicação entre dispositivos.

#### Compilar, gravar e monitorar

```bash
cd src/firmware/teste_navegacao

idf.py -p <porta> flash monitor
```

Substitua `<porta>` pela porta serial do seu sistema:
- **macOS:** `/dev/cu.usbserial-XXXX`
- **Linux:** `/dev/ttyUSB0`
- **Windows:** `COM3`

#### Selecionar o labirinto

Antes de iniciar a corrida, pressione o botão (GPIO 25) para alternar entre os modos:
- **1 bipe agudo** → labirinto 4×4
- **2 bipes graves** → labirinto 8×8

---

### 🧪 Testes

#### Backend

```bash
cd src/backend
source venv/bin/activate
PYTHONPATH=. pytest --cov=app --cov-report=term-missing
```

#### Frontend

```bash
cd src/frontend
npm run test
```

#### E2E (Playwright)

Com backend e frontend rodando:

```bash
cd src/frontend
npx playwright test
```
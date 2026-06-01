# Backend

Backend da aplicação desenvolvido com FastAPI.

---
## 📖 Descrição do Projeto
Backend responsável por receber, processar e disponibilizar dados de telemetria do sistema Micromouse.

---

## 📁 Estrutura do Projeto

```bash
backend/
├── app/
│   ├── routes/
│   │   ├── health.py
│   │   └── telemetria.py
│   ├── models/
│   ├── schemas/
│   └── main.py
├── tests/
│   ├── conftest.py
│   ├── test_health.py
│   └── test_telemetria.py
├── pytest.ini
├── requirements.txt
└── README.md
```

---

## 🚀 Como rodar localmente

### 1. Clonar o repositório

```bash
git clone https://github.com/fcte-pi1/2026_1_PI1_Grupo01_Juliana.git
```

---

### 2. Entrar na pasta do projeto

```bash
cd src/backend
```

---

### 3. Criar e ativar ambiente virtual

#### Linux/macOS

```bash
python -m venv venv
source venv/bin/activate
```

#### Windows

```bash
python -m venv venv
venv\Scripts\activate
```

---

### 4. Instalar as dependências

```bash
pip install -r requirements.txt
```

> Requer **Python 3.10+** (o `requirements.txt` fixa `alembic 1.18.4`,
> que não suporta versões anteriores).

---

### 5. Aplicar as migrations

```bash
PYTHONPATH=. alembic upgrade head
```

> O `PYTHONPATH=.` é necessário porque `migrations/env.py` importa
> `from app.database import Base`. Sem ele o alembic falha com
> `ModuleNotFoundError: No module named 'app'`. Alternativa equivalente:
> `python -m alembic upgrade head`.

(Opcional) Popular o banco com dados de exemplo:

```bash
PYTHONPATH=. python seed.py
```

---

### 6. Executar o servidor

```bash
uvicorn app.main:app --reload
```

A aplicação ficará disponível em:

```bash
http://localhost:8000
```

---

## ✅ Testes automatizados (pytest + cobertura)

A suíte de testes fica em `tests/` e roda sobre um **banco SQLite isolado em
memória** — não toca no `micromouse.db`. A configuração está no `pytest.ini`
(cobertura via `pytest-cov` já habilitada).

```bash
# com o venv ativado e dentro de src/backend
pip install -r requirements.txt   # garante pytest e pytest-cov
pytest                            # roda tudo + relatório de cobertura no terminal
```

O relatório HTML detalhado é gerado em `htmlcov/index.html`. Para rodar um
arquivo ou teste específico:

```bash
pytest tests/test_telemetria.py
pytest tests/test_telemetria.py::test_post_telemetria_labirinto_inexistente_retorna_404
```

### Fixtures disponíveis (em `tests/conftest.py`)

Use-as nos novos testes — basta declará-las como argumento da função:

| Fixture             | Para que serve                                                        |
| ------------------- | --------------------------------------------------------------------- |
| `client`            | `TestClient` da API já conectado ao banco de teste.                   |
| `db_session`        | Sessão SQLAlchemy para montar dados direto no banco.                  |
| `labirintos`        | Semeia os 3 labirintos e retorna `{"4x4": id, "8x8": id, "16x16": id}`. |
| `pacote_telemetria` | Construtor de payload de telemetria válido (aceita `**overrides`).    |

Exemplo (ver `tests/test_telemetria.py`):

```python
def test_exemplo(client, labirintos, pacote_telemetria):
    resp = client.post("/telemetria", json=pacote_telemetria(labirinto_id=labirintos["4x4"]))
    assert resp.status_code == 201
```

> A meta da entrega é **cobertura ≥ 70%**. Os testes-modelo já cobrem health e
> os casos básicos do `POST /telemetria`; as suítes completas (WebSocket,
> models/schemas, histórico) são as Tarefas T6–T9.

---

## 🧪 Como testar a API

### POST `/telemetria` pelo Swagger

O FastAPI já gera uma documentação interativa em
<http://localhost:8000/docs>.

1. Abra <http://localhost:8000/docs>.
2. Clique no endpoint `POST /telemetria`.
3. Clique em **Try it out**.
4. Cole o JSON abaixo no campo de body e clique em **Execute**:

   ```json
   {
     "corrida_id": 1,
     "timestamp": "2026-05-10T12:34:56Z",
     "posicao_x": 2,
     "posicao_y": 3,
     "nivel_bateria": 87.5,
     "velocidade": 0.42
   }
   ```

5. A resposta (status `201` + body com o evento criado) aparece logo
   abaixo.

### WebSocket `/ws/telemetria` pelo Postman

O Swagger não suporta WebSocket — use o Postman:

1. No Postman, clique em **New → WebSocket Request**.
2. Cole a URL: `ws://localhost:8000/ws/telemetria`.
3. Clique em **Connect**.
4. Na aba **Message**, selecione **JSON** e envie:

   ```json
   {
     "corrida_id": 1,
     "timestamp": "2026-05-10T12:36:00Z",
     "posicao_x": 5,
     "posicao_y": 5,
     "nivel_bateria": 70.0,
     "velocidade": 0.55
   }
   ```

5. O backend responde com o evento persistido (mesmo JSON com `id`
   gerado). Para ver o broadcast, abra **duas conexões** Postman para a
   mesma URL — ao enviar pacote em uma, ambas recebem.

---

## ✅ Endpoint de Health Check

Endpoint responsável por verificar se a API está online.

### Requisição

```http
GET /health
```

### Resposta esperada

```json
{
  "status": "OK"
}
```

---

## 📡 Telemetria

Recebimento dos pacotes enviados pelo micromouse durante uma corrida.
Há duas formas equivalentes: `POST /telemetria` para envios pontuais e
`WebSocket /ws/telemetria` para fluxo contínuo em tempo real (com
broadcast para todos os clientes conectados).

### Payload

```json
{
  "corrida_id": 1,
  "labirinto_id": 2,
  "timestamp": "2026-05-10T12:34:56Z",
  "posicao_x": 2,
  "posicao_y": 3,
  "nivel_bateria": 87.5,
  "velocidade": 0.42
}
```

| Campo           | Tipo    | Obrigatório | Observação                                              |
| --------------- | ------- | ----------- | ------------------------------------------------------- |
| `corrida_id`    | int     | condicional | Usa uma corrida já existente.                           |
| `labirinto_id`  | int     | condicional | Se `corrida_id` não for enviado, abre uma nova corrida. |
| `timestamp`     | ISO8601 | sim         | Momento da leitura.                                     |
| `posicao_x`     | int ≥0  | sim         | Coordenada na grade.                                    |
| `posicao_y`     | int ≥0  | sim         | Coordenada na grade.                                    |
| `nivel_bateria` | float   | sim         | Porcentagem 0–100.                                      |
| `velocidade`    | float   | não         | m/s.                                                    |

> Pelo menos um entre `corrida_id` e `labirinto_id` deve ser informado.

### POST `/telemetria`

Recebe um pacote pontual de telemetria. Resposta `201`:

```json
{
  "id": 4,
  "corrida_id": 1,
  "timestamp": "2026-05-10T12:34:56",
  "posicao_x": 2,
  "posicao_y": 3,
  "nivel_bateria": 87.5,
  "velocidade": 0.42
}
```

> Veja a seção [🧪 Como testar a API](#-como-testar-a-api) para o
> passo-a-passo no Swagger.

### WebSocket `/ws/telemetria`

- O cliente envia mensagens JSON com o mesmo payload do POST.
- Cada pacote válido é persistido e re-emitido para todos os clientes
  conectados (inclusive o produtor), permitindo dashboards em tempo real.
- Pacotes inválidos retornam `{"erro": "...", "detalhes": [...]}` apenas
  para o emissor.

> Veja a seção [🧪 Como testar a API](#-como-testar-a-api) para o
> passo-a-passo no Postman.

---

## 📦 Dependências

As dependências estão definidas no arquivo:

```bash
requirements.txt
```

### Principais bibliotecas utilizadas

- FastAPI
- Uvicorn
- Pydantic
- Python Dotenv
- HTTPX

---

## 🛠️ Tecnologias Utilizadas

- Python 3.x
- FastAPI
- Uvicorn
- Pydantic

---
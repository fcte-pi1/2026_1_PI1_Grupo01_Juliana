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
│   │   └── health.py
│   └── main.py
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

---

### 5. Executar o servidor

```bash
uvicorn app.main:app --reload
```

A aplicação ficará disponível em:

```bash
http://localhost:8000
```

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
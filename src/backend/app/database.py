import os
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker, declarative_base

# Lê a URL do banco da variável de ambiente e 
# se não estiver definida, usa SQLite local como padrão (arquivo micromouse.db).
DATABASE_URL = os.getenv("DATABASE_URL", "sqlite:///./micromouse.db")

# O SQLite exige check_same_thread=False para funcionar corretamente com o FastAPI,
# que opera com múltiplas threads. Para outros bancos (ex: PostgreSQL), nenhum
# argumento extra é necessário.
connect_args = {"check_same_thread": False} if DATABASE_URL.startswith("sqlite") else {}

# Cria o engine do SQLAlchemy, responsável por gerenciar a conexão com o banco.
engine = create_engine(DATABASE_URL, connect_args=connect_args)

# Fábrica de sessões. Cada sessão representa uma transação com o banco.
# autocommit=False e autoflush=False garantem controle manual das transações.
SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)

# Base para os models. Todas as classes de modelo herdam desta Base,
# permitindo que o SQLAlchemy mapeie as classes para tabelas no banco.
Base = declarative_base()

def get_db():
    """
    Dependency do FastAPI para injetar uma sessão de banco nas rotas.
    """
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()
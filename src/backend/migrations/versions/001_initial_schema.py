from alembic import op
import sqlalchemy as sa

# Alembic usa para controlar o histórico de migrations
revision = "001_initial_schema"
down_revision = None
branch_labels = None
depends_on = None

def upgrade() -> None:
    # labirintos
    op.create_table(
        "labirintos",
        sa.Column("id", sa.Integer(), primary_key=True, index=True),
        sa.Column(
            "dimensao",
            sa.Enum("4x4", "8x8", "16x16", name="dimensaolabirinto"),
            nullable=False,
        ),
    )

    # corridas
    op.create_table(
        "corridas",
        sa.Column("id", sa.Integer(), primary_key=True, index=True),
        sa.Column("labirinto_id", sa.Integer(), sa.ForeignKey("labirintos.id"), nullable=False),
        sa.Column("data", sa.DateTime(timezone=True), server_default=sa.func.now()),
        sa.Column("duracao", sa.Float(), nullable=True),
        sa.Column("velocidade_media", sa.Float(), nullable=True),
        sa.Column(
            "resultado",
            sa.Enum("concluida", "falha", "em_andamento", name="resultadocorrida"),
            nullable=False,
            server_default="em_andamento",
        ),
    )

    # eventos_telemetria
    op.create_table(
        "eventos_telemetria",
        sa.Column("id", sa.Integer(), primary_key=True, index=True),
        sa.Column("corrida_id", sa.Integer(), sa.ForeignKey("corridas.id"), nullable=False),
        sa.Column("timestamp", sa.DateTime(timezone=True), nullable=False),
        sa.Column("posicao_x", sa.Integer(), nullable=False),
        sa.Column("posicao_y", sa.Integer(), nullable=False),
        sa.Column("nivel_bateria", sa.Float(), nullable=False),
    )

def downgrade() -> None:
    op.drop_table("eventos_telemetria")
    op.drop_table("corridas")
    op.drop_table("labirintos")
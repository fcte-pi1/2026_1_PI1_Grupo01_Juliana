from alembic import op
import sqlalchemy as sa

revision = "003_add_orientacao_mensagem_telemetria"
down_revision = "002_add_velocidade_telemetria"
branch_labels = None
depends_on = None


def upgrade() -> None:
    op.add_column(
        "eventos_telemetria",
        sa.Column("orientacao", sa.String(), nullable=True),
    )
    op.add_column(
        "eventos_telemetria",
        sa.Column("mensagem", sa.String(), nullable=True),
    )


def downgrade() -> None:
    op.drop_column("eventos_telemetria", "mensagem")
    op.drop_column("eventos_telemetria", "orientacao")

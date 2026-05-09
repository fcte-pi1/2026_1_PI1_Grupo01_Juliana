from alembic import op
import sqlalchemy as sa

revision = "002_add_velocidade_telemetria"
down_revision = "001_initial_schema"
branch_labels = None
depends_on = None


def upgrade() -> None:
    op.add_column(
        "eventos_telemetria",
        sa.Column("velocidade", sa.Float(), nullable=True),
    )


def downgrade() -> None:
    op.drop_column("eventos_telemetria", "velocidade")

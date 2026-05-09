from datetime import datetime
from app.database import SessionLocal
from app.models.models import (
    Labirinto,
    Corrida,
    EventoTelemetria,
    DimensaoLabirinto,
    ResultadoCorrida
)

db = SessionLocal()

try:
    db.query(EventoTelemetria).delete()
    db.query(Corrida).delete()
    db.query(Labirinto).delete()

    lab_4x4 = Labirinto(dimensao=DimensaoLabirinto.quatro_x_quatro)
    lab_8x8 = Labirinto(dimensao=DimensaoLabirinto.oito_x_oito)
    lab_16x16 = Labirinto(dimensao=DimensaoLabirinto.dezesseis_x_dezesseis)

    db.add_all([lab_4x4, lab_8x8, lab_16x16])
    db.commit()

    corrida_1 = Corrida(
        labirinto_id=lab_4x4.id,
        duracao=12.5,
        velocidade_media=0.45,
        resultado=ResultadoCorrida.concluida
    )

    corrida_2 = Corrida(
        labirinto_id=lab_8x8.id,
        duracao=31.2,
        velocidade_media=0.38,
        resultado=ResultadoCorrida.falha
    )

    db.add_all([corrida_1, corrida_2])
    db.commit()

    eventos = [
        EventoTelemetria(
            corrida_id=corrida_1.id,
            timestamp=datetime.now(),
            posicao_x=0,
            posicao_y=0,
            nivel_bateria=100
        ),
        EventoTelemetria(
            corrida_id=corrida_1.id,
            timestamp=datetime.now(),
            posicao_x=1,
            posicao_y=0,
            nivel_bateria=98
        ),
        EventoTelemetria(
            corrida_id=corrida_2.id,
            timestamp=datetime.now(),
            posicao_x=0,
            posicao_y=0,
            nivel_bateria=100
        )
    ]

    db.add_all(eventos)
    db.commit()

    print("Seed executado com sucesso.")

finally:
    db.close()
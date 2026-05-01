# Guia de Contribuição

Obrigado por contribuir com o projeto! Este documento define as convenções e o fluxo de trabalho adotados pelo grupo para manter o repositório organizado e colaborativo.

---

## Sumário

- [Fluxo de trabalho com branches](#fluxo-de-trabalho-com-branches)
- [Padrão de commits](#padrão-de-commits)
- [Pull Requests](#pull-requests)
- [Issues](#issues)
- [Estrutura do repositório](#estrutura-do-repositório)

---

## Fluxo de trabalho com branches

Utilizamos o modelo **Feature Branch Workflow**: toda contribuição é feita em uma branch separada, nunca diretamente na `main`.

### Nomenclatura de branches

```
<tipo>/<descricao-curta>
```

| Tipo | Quando usar |
|------|-------------|
| `feature/` | Nova funcionalidade ou entrega |
| `fix/` | Correção de bug ou problema |
| `docs/` | Documentação |
| `hw/` | Alterações em hardware (esquemáticos, PCB) |
| `mec/` | Alterações em arquivos mecânicos (CAD) |
| `refactor/` | Refatoração de código sem mudança de comportamento |

**Exemplos:**
```
feature/sensor-ir-leitura
fix/motor-pwm-frequencia
docs/atualiza-readme
hw/esquematico-ponte-h
mec/chassi-v2
```

### Passo a passo

```bash
# 1. Garanta que sua main está atualizada
git checkout main
git pull origin main

# 2. Crie sua branch a partir da main
git checkout -b feature/nome-da-sua-feature

# 3. Faça suas alterações e commits
git add .
git commit -m "feat(escopo): descrição da mudança"

# 4. Suba a branch
git push origin feature/nome-da-sua-feature

# 5. Abra um Pull Request no GitHub
```

> Nunca dê `push` diretamente na `main`. A branch está protegida e exige PR com pelo menos 1 aprovação.

---

## Padrão de commits

Seguimos o padrão [Conventional Commits](https://www.conventionalcommits.org/pt-br/).

### Formato

```
<tipo>(<escopo>): <descrição curta> (#N)
```

O `(#N)` é opcional, mas recomendado quando o commit resolve ou avança uma issue.

### Tipos permitidos

| Tipo | Quando usar |
|------|-------------|
| `feat` | Nova funcionalidade |
| `fix` | Correção de bug |
| `docs` | Apenas documentação |
| `refactor` | Refatoração sem mudança de comportamento |
| `test` | Adição ou correção de testes |
| `chore` | Tarefas de manutenção (deps, configs) |
| `hw` | Alterações em hardware |
| `mec` | Alterações em arquivos mecânicos |

### Exemplos

```
feat(sensor): adiciona leitura de 8 sensores IR (#12)
fix(motor): corrige cálculo de duty cycle no PWM (#15)
docs(contributing): cria guia de contribuição (#1)
hw(esquematico): adiciona driver de motor DRV8833 (#8)
mec(chassi): atualiza modelo CAD com suporte do sensor (#10)
refactor(maze): simplifica algoritmo de flood fill (#20)
```

### Regras

- Use o imperativo na descrição: "adiciona", "corrige", "remove" (não "adicionado" ou "adicionando")
- Máximo de 72 caracteres na primeira linha
- Sem letra maiúscula no início da descrição
- Sem ponto final

---

## Pull Requests

### Quando abrir

Abra um PR assim que sua branch estiver pronta para revisão. Se ainda estiver em andamento, abra como **Draft PR**.

### Checklist antes de abrir

- [ ] A branch está atualizada com a `main`
- [ ] Os commits seguem o padrão Conventional Commits
- [ ] O código/arquivo foi testado localmente (ou no hardware, quando aplicável)
- [ ] A descrição do PR explica o que foi feito e por quê

### Preenchimento do PR

**Título:** siga o mesmo padrão de commit (`feat(escopo): descrição`)

**Descrição:** inclua:
- O que foi feito
- Como testar ou verificar
- Referência à issue relacionada (use `Closes #N` para fechar automaticamente)

### Revisão

- Todo PR precisa de **1 aprovação** antes do merge
- O autor do PR não pode aprovar o próprio PR
- Reviewer: deixe comentários construtivos e objetivos; aprove somente se estiver confortável com a mudança
- Após aprovação, o próprio autor faz o merge

---

## Issues

Usamos issues para rastrear tarefas, bugs e melhorias.

### Criando uma issue

- Use os templates disponíveis em `.github/ISSUE_TEMPLATE/`
- Adicione labels adequadas
- Atribua a um responsável se já souber quem vai pegar
- Vincule ao Project Board do grupo

### Labels principais

**Área técnica** — identifica o subgrupo responsável:

| Label | Descrição |
|-------|-----------|
| `software` | Tarefas da área de software |
| `hardware` | Tarefas da área de hardware |
| `embarcado` | Tarefas da área de embarcado |
| `estrutura` | Tarefas da área de estrutura mecânica |
| `energia` | Tarefas da área de energia |
| `integração` | Tarefas de integração entre subgrupos |
| `web` | Tarefas relacionadas à interface web |

**Tipo** — classifica a natureza da issue:

| Label | Descrição |
|-------|-----------|
| `bug` | Algo não está funcionando |
| `enhancement` | Nova funcionalidade ou melhoria |
| `doc` / `documentation` | Documentação (use `doc` como padrão) |
| `question` | Dúvida ou discussão |
| `geral` | Tarefas que não se encaixam em uma área específica |

**Sprint** — indica em qual sprint a issue está planejada:

| Label |
|-------|
| `sprint-1` |
| `sprint-2` |
| `sprint-3` |
| `sprint-4` |
| `sprint-5` |
| `sprint-6` |

**Status / triagem:**

| Label | Descrição |
|-------|-----------|
| `good first issue` | Boa para quem está começando no projeto |
| `help wanted` | Precisa de atenção extra ou colaboração |
| `duplicate` | Issue ou PR já existe |
| `invalid` | Não se aplica ao projeto |
| `wontfix` | Não será resolvido |

---

## Estrutura do repositório

```
.
├── docs/          # Documentação do projeto
├── hw/            # Hardware: esquemáticos, PCB, simulações
├── mec/           # Mecânica: arquivos CAD
├── src/           # Código-fonte
├── .github/       # Templates de issues e PRs
├── CONTRIBUTING.md
└── README.md
```

Arquivos grandes (vídeos, renders, etc.) não devem ser commitados. Suba em outra plataforma e adicione o link no README ou na issue correspondente.

---

Em caso de dúvidas, abra uma issue com a label `question` ou chame no grupo. Boas contribuições!